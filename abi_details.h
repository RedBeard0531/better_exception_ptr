#pragma once

#include <optional>
#include <typeinfo>

#if defined(_WIN32)

#include <Windows.h>

#include "third_party/MSVC/eh_details.h"
#include <memory>

namespace stdx::detail {

EHExceptionRecord* get_repr(const std::exception_ptr& e_ptr) {
    return reinterpret_cast<const std::shared_ptr<EHExceptionRecord>&>(e_ptr).get();
}

void* get_raw_ptr(const std::exception_ptr& e_ptr) {
    return get_repr(e_ptr)->params.pExceptionObject;
}

std::type_info* type(const std::exception_ptr& e_ptr) {
    auto record = get_repr(e_ptr);

#ifdef _EH_RELATIVE_OFFSETS
    auto base = (const char*)(record->params.pThrowImageBase);
#else
    auto base = ptrdiff_t(0);
#endif

    auto throwInfo = (ThrowInfo*)DecodePointer((void*)(record->params.pThrowInfo));
    auto cta = (CatchableTypeArray*)(base + throwInfo->pCatchableTypeArray);
    // Assume the current type is always the first CatchableType.
    auto ct0 = (CatchableType*)(base + cta->arrayOfCatchableTypes[0]);
    auto td = (TypeDescriptor*)(base + ct0->pType);
    return reinterpret_cast<std::type_info*>(td);
}

std::optional<void*> try_catch(const std::exception_ptr& e_ptr, const std::type_info* target_type) {
    auto record = get_repr(e_ptr);

#ifdef _EH_RELATIVE_OFFSETS
    auto base = (const char*)(record->params.pThrowImageBase);
#else
    auto base = ptrdiff_t(0);
#endif

    auto throwInfo = (ThrowInfo*)DecodePointer((void*)(record->params.pThrowInfo));
    auto cta = (CatchableTypeArray*)(base + throwInfo->pCatchableTypeArray);
    for (size_t i = 0; i < cta->nCatchableTypes; i++) {
        auto ct = (CatchableType*)(base + cta->arrayOfCatchableTypes[i]);
        auto td = (TypeDescriptor*)(base + ct->pType);
        if (td == (TypeDescriptor*)target_type) {
            return {adjustThis(ct->thisDisplacement, get_raw_ptr(e_ptr))};
        }
    }
    // No Match :(
    return {};
}

}

#else

#include "third_party/libsupc++/unwind-cxx.h"

namespace stdx::detail {

void* get_raw_ptr(const std::exception_ptr& e_ptr) {
    // exception_ptr is a void* to exception object.
    return reinterpret_cast<void* const &>(e_ptr);
}

std::type_info* type(const std::exception_ptr& e_ptr) {
    const auto obj_ptr = get_raw_ptr(e_ptr);
    const auto& header = __cxxabiv1::__get_refcounted_exception_header_from_obj(obj_ptr)->exc;
    return header.exceptionType;
}

std::optional<void*> try_catch(const std::exception_ptr& e_ptr, const std::type_info* target_type) {
    auto out = get_raw_ptr(e_ptr);
    // TODO pointer case.
    if (target_type->__do_catch(type(e_ptr), &out, 1))
        return out;
    return std::nullopt;
}

}

#endif
