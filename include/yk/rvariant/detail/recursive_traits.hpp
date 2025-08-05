#ifndef YK_RVARIANT_DETAIL_RECURSIVE_TRAITS_HPP
#define YK_RVARIANT_DETAIL_RECURSIVE_TRAITS_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/core/type_traits.hpp>

namespace yk::detail {

template<bool Found, std::size_t I, class U, class... Ts>
struct select_maybe_wrapped_impl;

template<std::size_t I, class U, class... Rest>
struct select_maybe_wrapped_impl<false, I, U, U, Rest...>
{
    using type = U;
    static constexpr std::size_t index = I;
};

template<std::size_t I, class U, class Allocator, class... Rest>
struct select_maybe_wrapped_impl<false, I, U, recursive_wrapper<U, Allocator>, Rest...>
{
    using type = recursive_wrapper<U, Allocator>;
    static constexpr std::size_t index = I;
};

template<std::size_t I, class U, class First, class... Rest>
struct select_maybe_wrapped_impl<false, I, U, First, Rest...>
    : select_maybe_wrapped_impl<false, I + 1, U, Rest...>
{};

template<class U, class... Ts>
struct select_maybe_wrapped : select_maybe_wrapped_impl<false, 0, U, Ts...>
{
    // Precondition: either T or recursive_wrapper<T> occurs at least once in Ts...
    static_assert(sizeof...(Ts) > 0);
    static_assert(!core::is_ttp_specialization_of_v<U, recursive_wrapper>);
};

template<class U, class... Ts>
using select_maybe_wrapped_t = typename select_maybe_wrapped<U, Ts...>::type;

template<class U, class... Ts>
constexpr std::size_t select_maybe_wrapped_index = select_maybe_wrapped<U, Ts...>::index;

} // yk::detail

#endif
