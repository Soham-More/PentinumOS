#pragma once

#include <includes.h>

namespace std
{
    template<typename T>
    class initializer_list
    {
    public:
        using value_type = T;
        using reference = const T&;
        using const_reference = const T&;
        using size_type = size_t;
        using iterator = const T*;
        using const_iterator = const T*;

    private:
        iterator  m_array;
        size_type m_len;

        // The compiler can call a private constructor.
        constexpr initializer_list(const_iterator itr, size_type st)
            : m_array(itr), m_len(st) { }

    public:
        constexpr initializer_list() noexcept : m_array(0), m_len(0) { }

        // Number of elements.
        constexpr size_type size() const noexcept { return m_len; }

        // First element.
        constexpr const_iterator begin() const noexcept { return m_array; }

        // One past the last element.
        constexpr const_iterator end() const noexcept { return begin() + size(); }
    };
}