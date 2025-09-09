#pragma once

#include <includes.h>
#include <io/io.h>
#include "../errors.hpp"

namespace std
{
    template<typename T>
    struct result_reference
    {
        u32 error_code;
        T* value;

        result_reference() : value(nullptr), error_code(0) {}
        result_reference(u32 _error) : value(nullptr), error_code(_error) {}
        result_reference(T* ptr) { value = ptr; error_code = 0; }

        bool is_valid() { return error_code != 0; }

        T& expect(const char* panic_str)
        {
            if(!value)
            {
                log_error("[PANIC][std][error code: 0x%x] %s", panic_str, error_code);
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
            if(!value) return or_function(error_code);
            return *value;
        }

        T& unsafe_get() { return *value; }
    };

    template<typename T>
    inline result_reference<T> Err(u32 err) { return result_reference<T>(err); }
    template<typename T>
    inline result_reference<T> Ok(T& _value) { return result_reference<T>(&_value); }
}

