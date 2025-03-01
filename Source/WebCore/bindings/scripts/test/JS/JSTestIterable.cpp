/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSTestIterable.h"

#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMIterator.h"
#include "JSDOMOperation.h"
#include "JSDOMWrapperCache.h"
#include <builtins/BuiltinNames.h>
#include <runtime/FunctionPrototype.h>
#include <runtime/JSCInlines.h>
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

// Functions

JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionSymbolIterator(JSC::ExecState*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionEntries(JSC::ExecState*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionKeys(JSC::ExecState*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionValues(JSC::ExecState*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionForEach(JSC::ExecState*);

// Attributes

JSC::EncodedJSValue jsTestIterableConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestIterableConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::EncodedJSValue);

class JSTestIterablePrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestIterablePrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestIterablePrototype* ptr = new (NotNull, JSC::allocateCell<JSTestIterablePrototype>(vm.heap)) JSTestIterablePrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestIterablePrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

using JSTestIterableConstructor = JSDOMConstructorNotConstructable<JSTestIterable>;

template<> JSValue JSTestIterableConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestIterableConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestIterable::prototype(vm, globalObject), DontDelete | ReadOnly | DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(&vm, String(ASCIILiteral("TestIterable"))), ReadOnly | DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), ReadOnly | DontEnum);
}

template<> const ClassInfo JSTestIterableConstructor::s_info = { "TestIterable", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestIterableConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestIterablePrototypeTableValues[] =
{
    { "constructor", DontEnum, NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestIterableConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestIterableConstructor) } },
    { "entries", JSC::Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(jsTestIterablePrototypeFunctionEntries), (intptr_t) (0) } },
    { "keys", JSC::Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(jsTestIterablePrototypeFunctionKeys), (intptr_t) (0) } },
    { "values", JSC::Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(jsTestIterablePrototypeFunctionValues), (intptr_t) (0) } },
    { "forEach", JSC::Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(jsTestIterablePrototypeFunctionForEach), (intptr_t) (1) } },
};

const ClassInfo JSTestIterablePrototype::s_info = { "TestIterablePrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestIterablePrototype) };

void JSTestIterablePrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestIterablePrototypeTableValues, *this);
    putDirect(vm, vm.propertyNames->iteratorSymbol, getDirect(vm, vm.propertyNames->builtinNames().entriesPublicName()), DontEnum);
}

const ClassInfo JSTestIterable::s_info = { "TestIterable", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestIterable) };

JSTestIterable::JSTestIterable(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestIterable>&& impl)
    : JSDOMWrapper<TestIterable>(structure, globalObject, WTFMove(impl))
{
}

void JSTestIterable::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

}

JSObject* JSTestIterable::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestIterablePrototype::create(vm, &globalObject, JSTestIterablePrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestIterable::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestIterable>(vm, globalObject);
}

JSValue JSTestIterable::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestIterableConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestIterable::destroy(JSC::JSCell* cell)
{
    JSTestIterable* thisObject = static_cast<JSTestIterable*>(cell);
    thisObject->JSTestIterable::~JSTestIterable();
}

template<> inline JSTestIterable* IDLOperation<JSTestIterable>::cast(ExecState& state)
{
    return jsDynamicDowncast<JSTestIterable*>(state.vm(), state.thisValue());
}

EncodedJSValue jsTestIterableConstructor(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicDowncast<JSTestIterablePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(state, throwScope);
    return JSValue::encode(JSTestIterable::getConstructor(state->vm(), prototype->globalObject()));
}

bool setJSTestIterableConstructor(ExecState* state, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicDowncast<JSTestIterablePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(state, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(state->vm(), state->propertyNames().constructor, JSValue::decode(encodedValue));
}

struct TestIterableIteratorTraits {
    static constexpr JSDOMIteratorType type = JSDOMIteratorType::Set;
    using KeyType = void;
    using ValueType = IDLInterface<TestNode>;
};

using TestIterableIterator = JSDOMIterator<JSTestIterable, TestIterableIteratorTraits>;
using TestIterableIteratorPrototype = JSDOMIteratorPrototype<JSTestIterable, TestIterableIteratorTraits>;

template<>
const JSC::ClassInfo TestIterableIterator::s_info = { "TestIterable Iterator", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(TestIterableIterator) };

template<>
const JSC::ClassInfo TestIterableIteratorPrototype::s_info = { "TestIterable Iterator", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(TestIterableIteratorPrototype) };

static inline EncodedJSValue jsTestIterablePrototypeFunctionEntriesCaller(ExecState*, JSTestIterable* thisObject, JSC::ThrowScope&)
{
    return JSValue::encode(iteratorCreate<TestIterableIterator>(*thisObject, IterationKind::Value));
}

JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionEntries(JSC::ExecState* state)
{
    return IDLOperation<JSTestIterable>::call<jsTestIterablePrototypeFunctionEntriesCaller>(*state, "entries");
}

static inline EncodedJSValue jsTestIterablePrototypeFunctionKeysCaller(ExecState*, JSTestIterable* thisObject, JSC::ThrowScope&)
{
    return JSValue::encode(iteratorCreate<TestIterableIterator>(*thisObject, IterationKind::Key));
}

JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionKeys(JSC::ExecState* state)
{
    return IDLOperation<JSTestIterable>::call<jsTestIterablePrototypeFunctionKeysCaller>(*state, "keys");
}

static inline EncodedJSValue jsTestIterablePrototypeFunctionValuesCaller(ExecState*, JSTestIterable* thisObject, JSC::ThrowScope&)
{
    return JSValue::encode(iteratorCreate<TestIterableIterator>(*thisObject, IterationKind::Value));
}

JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionValues(JSC::ExecState* state)
{
    return IDLOperation<JSTestIterable>::call<jsTestIterablePrototypeFunctionValuesCaller>(*state, "values");
}

static inline EncodedJSValue jsTestIterablePrototypeFunctionForEachCaller(ExecState* state, JSTestIterable* thisObject, JSC::ThrowScope& throwScope)
{
    return JSValue::encode(iteratorForEach<TestIterableIterator>(*state, *thisObject, throwScope));
}

JSC::EncodedJSValue JSC_HOST_CALL jsTestIterablePrototypeFunctionForEach(JSC::ExecState* state)
{
    return IDLOperation<JSTestIterable>::call<jsTestIterablePrototypeFunctionForEachCaller>(*state, "forEach");
}

bool JSTestIterableOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    return false;
}

void JSTestIterableOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestIterable = static_cast<JSTestIterable*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestIterable->wrapped(), jsTestIterable);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestIterable@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore12TestIterableE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject* globalObject, Ref<TestIterable>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = reinterpret_cast<void*>(__identifier("??_7TestIterable@WebCore@@6B@"));
#else
    void* expectedVTablePointer = &_ZTVN7WebCore12TestIterableE[2];
#if COMPILER(CLANG)
    // If this fails TestIterable does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(__is_polymorphic(TestIterable), "TestIterable is not polymorphic");
#endif
#endif
    // If you hit this assertion you either have a use after free bug, or
    // TestIterable has subclasses. If TestIterable has subclasses that get passed
    // to toJS() we currently require TestIterable you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestIterable>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestIterable& impl)
{
    return wrap(state, globalObject, impl);
}

TestIterable* JSTestIterable::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicDowncast<JSTestIterable*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}
