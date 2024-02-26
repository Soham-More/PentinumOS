#pragma once

namespace std
{
    template <typename T>
    struct remove_reference {
        using type = T;
    };
    template <typename T>
    struct remove_reference<T&> {
        using type = T;
    };
    template <typename T>
    struct remove_reference<T&&> {
        using type = T;
    };

    template <typename T>
    using remove_reference_t = typename remove_reference<T>::type;

    template <typename T>
    constexpr std::remove_reference_t<T>&& move(T&& t)
    {
        return static_cast<std::remove_reference_t<T>&&>(t);
    }
}


