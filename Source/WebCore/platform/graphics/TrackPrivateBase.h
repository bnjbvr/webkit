/*
 * Copyright (C) 2012-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Cable Television Labs, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(VIDEO_TRACK)

#include <wtf/MediaTime.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class TrackPrivateBaseClient {
public:
    virtual ~TrackPrivateBaseClient() { }
    virtual void idChanged(const AtomicString&) = 0;
    virtual void labelChanged(const AtomicString&) = 0;
    virtual void languageChanged(const AtomicString&) = 0;
    virtual void willRemove() = 0;
};

class TrackPrivateBase : public ThreadSafeRefCounted<TrackPrivateBase> {
    WTF_MAKE_NONCOPYABLE(TrackPrivateBase);
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~TrackPrivateBase() = default;

    virtual TrackPrivateBaseClient* client() const = 0;

    virtual AtomicString id() const { return emptyAtom; }
    virtual AtomicString label() const { return emptyAtom; }
    virtual AtomicString language() const { return emptyAtom; }

    virtual int trackIndex() const { return 0; }

    virtual MediaTime startTimeVariance() const { return MediaTime::zeroTime(); }

    void willBeRemoved()
    {
        if (auto* client = this->client())
            client->willRemove();
    }

protected:
    TrackPrivateBase() = default;
};

} // namespace WebCore

#endif
