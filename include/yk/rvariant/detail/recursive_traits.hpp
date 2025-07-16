#ifndef YK_RVARIANT_DETAIL_RECURSIVE_TRAITS_HPP
#define YK_RVARIANT_DETAIL_RECURSIVE_TRAITS_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/detail/lang_core.hpp>

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


template<class Sentinel, class T>
struct recursively_constructible;

template<class Sentinel, class T>
constexpr bool recursively_constructible_v = recursively_constructible<Sentinel, T>::value;


template<class ArgsList, class... TestedVariants>
struct recursive_sentinel_t;

template<class... Args, class... TestedVariants>
struct recursive_sentinel_t<type_list<Args...>, TestedVariants...>
{
    constexpr explicit recursive_sentinel_t() = default;

    //template<class T>
    //    requires
    //        (!ttp_specialization_of<T, rvariant>) &&
    //        std::is_constructible_v<T, Args...>
    //constexpr operator T&& ();

    //template<class T>
    //    requires
    //        (ttp_specialization_of<T, rvariant>) &&
    //        recursively_constructible<recursive_sentinel_t, T>::value
    //constexpr operator T&& ();
};

template<class ArgsList, class... TestedVariants>
constexpr recursive_sentinel_t<ArgsList, TestedVariants...> recursive_sentinel{};



template<class... TestedVariants, class NonVariant, class... Args>
struct recursively_constructible<
    recursive_sentinel_t<type_list<Args...>, TestedVariants...>,
    NonVariant
> : std::is_constructible<NonVariant, recursive_sentinel_t<type_list<Args...>, TestedVariants...>>
{};

template<class... TestedVariants, class... Receiver, class... Args>
struct recursively_constructible<
    recursive_sentinel_t<type_list<Args...>, TestedVariants...>,
    rvariant<Receiver...>
> : std::disjunction<
    std::conditional_t<
        is_in_v<Receiver, TestedVariants...>,
        std::false_type, // already tested
        std::is_constructible<Receiver, recursive_sentinel_t<type_list<Args...>, TestedVariants..., rvariant<Receiver...>>>
    >...
>
{};

} // yk::detail

#endif
