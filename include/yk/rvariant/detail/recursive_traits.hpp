#ifndef YK_RVARIANT_DETAIL_RECURSIVE_TRAITS_HPP
#define YK_RVARIANT_DETAIL_RECURSIVE_TRAITS_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/core/type_traits.hpp>

namespace yk::detail {

template<class W, class T>
struct is_any_wrapper_of : std::false_type {};

template<class T, class Allocator>
struct is_any_wrapper_of<recursive_wrapper<T, Allocator>, T> : std::true_type {};

template<class W, class T>
constexpr bool is_any_wrapper_of_v = is_any_wrapper_of<W, T>::value;


template<bool Found, std::size_t I, class U, class... Ts>
struct select_maybe_wrapped_impl;

template<std::size_t I, class U>
struct select_maybe_wrapped_impl<true, I, U>
{
    using type = U;
    static constexpr std::size_t index = I;
};

template<std::size_t I, class U, class First, class... Rest>
struct select_maybe_wrapped_impl<false, I, U, First, Rest...>
    : std::conditional_t<
        std::is_same_v<First, U>,
        select_maybe_wrapped_impl<true, I, U>,
        std::conditional_t<
            is_any_wrapper_of_v<First, U>,
            select_maybe_wrapped_impl<true, I, First>,
            select_maybe_wrapped_impl<false, I + 1, U, Rest...>
        >
    >
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
