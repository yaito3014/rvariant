#ifndef YK_FORMAT_TRAITS_HPP
#define YK_FORMAT_TRAITS_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <format>
#include <type_traits>


namespace yk {

template<class charT>
struct format_traits;

template<>
struct format_traits<char>
{
    using char_type = char;
    static constexpr char_type brace_open = '{';
    static constexpr char_type brace_close = '}';

    template<class T>
    static constexpr std::basic_format_string<char_type, std::type_identity_t<T>>
        brace_full = "{}";
};

template<>
struct format_traits<wchar_t>
{
    using char_type = wchar_t;
    static constexpr char_type brace_open = L'{';
    static constexpr char_type brace_close = L'}';

    template<class T>
    static constexpr std::basic_format_string<char_type, std::type_identity_t<T>>
        brace_full = L"{}";
};

namespace detail {

template<class... CharLike>
struct select_char_t_impl
{
    static_assert(sizeof...(CharLike) > 0);
    // ReSharper disable once CppStaticAssertFailure
    static_assert(
        false,
        "All char-like arguments must be convertible to the identical "
        "`std::basic_string_view` variant for the same char type."
    );
};

template<class... CharLike>
    requires std::conjunction_v<std::is_convertible<CharLike, std::basic_string_view<char>>...>
struct select_char_t_impl<CharLike...>
{
    using type = char;
};

template<class... CharLike>
    requires std::conjunction_v<std::is_convertible<CharLike, std::basic_string_view<wchar_t>>...>
struct select_char_t_impl<CharLike...>
{
    using type = wchar_t;
};

} // detail

template<class... CharLike>
using select_char_t = typename detail::select_char_t_impl<CharLike...>::type;

} // yk

#endif
