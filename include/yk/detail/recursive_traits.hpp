#ifndef YK_DETAIL_RECURSIVE_TRAITS_HPP
#define YK_DETAIL_RECURSIVE_TRAITS_HPP

#include "yk/detail/pack_indexing.hpp"
#include "yk/detail/is_specialization_of.hpp"
#include "yk/detail/is_in.hpp"

#include <type_traits>
#include <concepts>


namespace yk {

template<class... Ts>
class rvariant;

template<class T, class Allocator>
class recursive_wrapper;

} // yk

namespace yk::detail {

template<bool Found, std::size_t I, class T, class... Ts>
struct select_maybe_wrapped_impl;

template<std::size_t I, class T>
struct select_maybe_wrapped_impl<true, I, T>
{
    using type = T;
    static constexpr std::size_t index = I;
};

template<std::size_t I, class T, class First, class... Rest>
struct select_maybe_wrapped_impl<false, I, T, First, Rest...>
    : std::conditional_t<
        std::is_same_v<First, T>, select_maybe_wrapped_impl<true, I, T>,
        std::conditional_t<
            std::is_same_v<First, recursive_wrapper<T>>, select_maybe_wrapped_impl<true, I, recursive_wrapper<T>>,
            select_maybe_wrapped_impl<false, I + 1, T, Rest...>
        >
    >
{};

template<class T, class... Ts>
struct select_maybe_wrapped : select_maybe_wrapped_impl<false, 0, T, Ts...>
{
    // Precondition: either T or recursive_wrapper<T> occurs at least once in Ts...
    static_assert(sizeof...(Ts) > 0);
    static_assert(!ttp_specialization_of<T, recursive_wrapper>);
};

template<class T, class... Ts>
using select_maybe_wrapped_t = typename select_maybe_wrapped<T, Ts...>::type;

template<class T, class... Ts>
constexpr std::size_t select_maybe_wrapped_index = select_maybe_wrapped<T, Ts...>::index;

} // yk::detail

#endif
