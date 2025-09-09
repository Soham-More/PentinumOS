#pragma once

#include <includes.h>
#include <io/io.h>
#include "../errors.hpp"

namespace std
{
    template<typename T>
    struct optional_reference
    {
        T* value;

        optional_reference() : value(nullptr) {}
        optional_reference(T* ptr) { value = ptr; }

        bool is_valid() { return value; }

        T& expect(const char* panic_str)
        {
            if(!value)
            {
                log_error("[PANIC][std] %s", panic_str);
                x86_raise(STD_UNWRAP_FAIL);
            }
            return *value;
        }

        T& unwrap()
        {
            if(!value) x86_raise(STD_UNWRAP_FAIL);
            return *value;
        }

        T& unwrap_or(T& default_reference)
        {
            if(!value) return default_reference;
            return *value;
        }

        template<typename F>
        T& unwrap_or(F or_function)
        {
            if(!value) return or_function();
            return *value;
        }

        T& unsafe_get() { return *value; }
    };

    template<typename T>
    inline optional_reference<T> None() { return optional_reference<T>(); }
    template<typename T>
    inline optional_reference<T> Some(T& _value) { return optional_reference<T>(&_value); }
}

