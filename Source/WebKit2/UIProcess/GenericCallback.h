/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "APIError.h"
#include "APISerializedScriptValue.h"
#include "ProcessThrottler.h"
#include "ShareableBitmap.h"
#include "WKAPICast.h"
#include <wtf/Function.h>
#include <wtf/HashMap.h>
#include <wtf/MainThread.h>
#include <wtf/RefCounted.h>
#include <wtf/RunLoop.h>
#include <wtf/Threading.h>

namespace WebKit {

class CallbackBase : public RefCounted<CallbackBase> {
public:
    enum class Error {
        None,
        Unknown,
        ProcessExited,
        OwnerWasInvalidated,
    };

    virtual ~CallbackBase()
    {
    }

    uint64_t callbackID() const { return m_callbackID; }

    template<class T>
    T* as()
    {
        if (T::type() == m_type)
            return static_cast<T*>(this);

        return nullptr;
    }

    virtual void invalidate(Error) = 0;

protected:
    struct TypeTag { };
    typedef const TypeTag* Type;

    explicit CallbackBase(Type type, const ProcessThrottler::BackgroundActivityToken& activityToken)
        : m_type(type)
        , m_callbackID(generateCallbackID())
        , m_activityToken(activityToken)
    {
    }

private:
    static uint64_t generateCallbackID()
    {
        ASSERT(RunLoop::isMain());
        static uint64_t uniqueCallbackID = 1;
        return uniqueCallbackID++;
    }

    Type m_type;
    uint64_t m_callbackID;
    ProcessThrottler::BackgroundActivityToken m_activityToken;
};

template<typename... T>
class GenericCallback : public CallbackBase {
public:
    typedef Function<void (T..., Error)> CallbackFunction;

    static Ref<GenericCallback> create(CallbackFunction&& callback, const ProcessThrottler::BackgroundActivityToken& activityToken = nullptr)
    {
        return adoptRef(*new GenericCallback(WTFMove(callback), activityToken));
    }

    virtual ~GenericCallback()
    {
        ASSERT(currentThread() == m_originThreadID);
        ASSERT(!m_callback);
    }

    void performCallbackWithReturnValue(T... returnValue)
    {
        ASSERT(currentThread() == m_originThreadID);

        if (!m_callback)
            return;

        m_callback.value()(returnValue..., Error::None);

        m_callback = std::nullopt;
    }

    void performCallback()
    {
        performCallbackWithReturnValue();
    }

    void invalidate(Error error = Error::Unknown) final
    {
        ASSERT(currentThread() == m_originThreadID);

        if (!m_callback)
            return;

        m_callback.value()(typename std::remove_reference<T>::type()..., error);

        m_callback = std::nullopt;
    }

private:
    GenericCallback(CallbackFunction&& callback, const ProcessThrottler::BackgroundActivityToken& activityToken)
        : CallbackBase(type(), activityToken)
        , m_callback(WTFMove(callback))
    {
    }

    friend class CallbackBase;
    static Type type()
    {
        static TypeTag tag;
        return &tag;
    }

    std::optional<CallbackFunction> m_callback;

#ifndef NDEBUG
    ThreadIdentifier m_originThreadID { currentThread() };
#endif
};

template<typename APIReturnValueType, typename InternalReturnValueType = typename APITypeInfo<APIReturnValueType>::ImplType*>
static typename GenericCallback<InternalReturnValueType>::CallbackFunction toGenericCallbackFunction(void* context, void (*callback)(APIReturnValueType, WKErrorRef, void*))
{
    return [context, callback](InternalReturnValueType returnValue, CallbackBase::Error error) {
        callback(toAPI(returnValue), error != CallbackBase::Error::None ? toAPI(API::Error::create().ptr()) : 0, context);
    };
}

typedef GenericCallback<> VoidCallback;
typedef GenericCallback<const Vector<WebCore::IntRect>&, double> ComputedPagesCallback;
typedef GenericCallback<const ShareableBitmap::Handle&> ImageCallback;

template<typename T>
void invalidateCallbackMap(HashMap<uint64_t, T>& callbackMap, CallbackBase::Error error)
{
    Vector<T> callbacks;
    copyValuesToVector(callbackMap, callbacks);
    for (auto& callback : callbacks)
        callback->invalidate(error);

    callbackMap.clear();
}

class CallbackMap {
public:
    uint64_t put(Ref<CallbackBase>&& callback)
    {
        RELEASE_ASSERT(RunLoop::isMain());
        uint64_t callbackID = callback->callbackID();
        RELEASE_ASSERT(callbackID);
        RELEASE_ASSERT(!m_map.contains(callbackID));
        m_map.set(callbackID, WTFMove(callback));
        return callbackID;
    }

    template<unsigned I, typename T, typename... U>
    struct GenericCallbackType {
        typedef typename GenericCallbackType<I - 1, U..., T>::type type;
    };

    template<typename... U>
    struct GenericCallbackType<1, CallbackBase::Error, U...> {
        typedef GenericCallback<U...> type;
    };

    template<typename... T>
    uint64_t put(Function<void(T...)>&& function, const ProcessThrottler::BackgroundActivityToken& activityToken)
    {
        auto callback = GenericCallbackType<sizeof...(T), T...>::type::create(WTFMove(function), activityToken);
        return put(WTFMove(callback));
    }

    template<class T>
    RefPtr<T> take(uint64_t callbackID)
    {
        RELEASE_ASSERT(callbackID);
        RELEASE_ASSERT(RunLoop::isMain());
        auto base = m_map.take(callbackID);
        if (!base)
            return nullptr;

        return adoptRef(base.leakRef()->as<T>());
    }

    void invalidate(CallbackBase::Error error)
    {
        RELEASE_ASSERT(RunLoop::isMain());
        invalidateCallbackMap(m_map, error);
    }

private:
    HashMap<uint64_t, RefPtr<CallbackBase>> m_map;
};

} // namespace WebKit
