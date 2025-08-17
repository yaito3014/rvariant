#ifndef YK_CORE_TYPE_TRAITS_HPP
#define YK_CORE_TYPE_TRAITS_HPP

// Copyright 2025 Nana Sakisaka
// Copyright 2025 Yaito Kakeyama
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// ReSharper disable once CppUnusedIncludeDirective
#include <yk/core/config.hpp>
// ReSharper disable once CppUnusedIncludeDirective
#include <yk/core/requirements.hpp>

#include <type_traits>
#include <utility>

#include <cstddef>

namespace yk::core {

template<class T, template<class...> class TT>
struct is_ttp_specialization_of : std::false_type {};

template<template<class...> class TT, class... Ts>
struct is_ttp_specialization_of<TT<Ts...>, TT> : std::true_type {};

template<class T, template<class...> class TT>
inline constexpr bool is_ttp_specialization_of_v = is_ttp_specialization_of<T, TT>::value;


template<class T, template<auto...> class TT>
struct is_nttp_specialization_of : std::false_type {};

template<template<auto...> class TT, auto... Ts>
struct is_nttp_specialization_of<TT<Ts...>, TT> : std::true_type {};

template<class T, template<auto...> class TT>
inline constexpr bool is_nttp_specialization_of_v = is_nttp_specialization_of<T, TT>::value;


template<class... Ts>
struct type_list
{
    static constexpr std::size_t size = sizeof...(Ts);
};

template<class T, class> struct pack_identity { using type = T; };
template<class T, class> using pack_identity_t = T;

template<class T, auto> struct npack_identity { using type = T; };
template<class T, auto> using npack_identity_t = T;

template<auto> using nvoid_t = void;


namespace detail {

template<class Voids>
struct do_pack_indexing;

template<std::size_t... Voids>
struct do_pack_indexing<std::index_sequence<Voids...>>
{
    template<class T>
    static T select(nvoid_t<Voids>*..., std::type_identity<T>*, ...);
};

template<class Voids>
struct do_npack_indexing;

template<std::size_t... Voids>
struct do_npack_indexing<std::index_sequence<Voids...>>
{
    template<class T, T N>
    static std::integral_constant<T, N> select(nvoid_t<Voids>*..., std::integral_constant<T, N>*, ...);
};

} // detail


template<std::size_t I, class T>
struct at_c;

template<std::size_t I, class T>
using at_c_t = typename at_c<I, T>::type;

#if __cpp_pack_indexing >= 202311L // has native pack indexing
# if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wc++26-extensions"
# endif

# define YK_CORE_PACK_INDEXING(I, Ts_ellipsis) Ts_ellipsis[I]

template<std::size_t I, class... Ts> struct pack_indexing { using type = Ts...[I]; };
template<std::size_t I, class... Ts> using pack_indexing_t = Ts...[I];
template<std::size_t I, auto... Ns> struct npack_indexing { static constexpr auto value = Ns...[I]; };
template<std::size_t I, auto... Ns> constexpr auto npack_indexing_v = Ns...[I];

template<std::size_t I, template<class...> class TT, class... Ts>
struct at_c<I, TT<Ts...>>
{
    using type = Ts...[I];
};
# if defined(__clang__)
#  pragma clang diagnostic pop
# endif
// ----------------------------------------------------------
#else // no native pack indexing
template<std::size_t I, class... Ts>
struct pack_indexing
{
    static_assert(I < sizeof...(Ts));
    using type = decltype(detail::do_pack_indexing<std::make_index_sequence<I>>::select(
        static_cast<std::type_identity<Ts>*>(nullptr)...
    ));
};

template<std::size_t I, class... Ts>
using pack_indexing_t = typename pack_indexing<I, Ts...>::type;

# define YK_CORE_PACK_INDEXING(I, Ts_ellipsis) ::yk::core::pack_indexing_t<I, Ts_ellipsis>


template<std::size_t I, auto... Ns>
struct npack_indexing
{
    static_assert(I < sizeof...(Ns));
    static constexpr auto value = decltype(detail::do_npack_indexing<std::make_index_sequence<I>>::select(
        static_cast<std::integral_constant<decltype(Ns), Ns>*>(nullptr)...
    ))::value;
};

template<std::size_t I, auto... Ns>
constexpr auto npack_indexing_v = npack_indexing<I, Ns...>::value;


template<std::size_t I, template<class...> class TT, class... Ts>
struct at_c<I, TT<Ts...>>
{
    static_assert(I < sizeof...(Ts));
    using type = decltype(detail::do_pack_indexing<std::make_index_sequence<I>>::select(
        static_cast<std::type_identity<Ts>*>(nullptr)...
    ));
};
#endif

// ----------------------------------------------------------


inline constexpr std::size_t find_npos = static_cast<std::size_t>(-1);

namespace detail {

template<std::size_t I, class T, class... Ts>
struct find_index_impl
    : std::integral_constant<std::size_t, find_npos>
{};

template<std::size_t I, class T, class U, class... Us>
struct find_index_impl<I, T, U, Us...>
    : std::conditional_t<
        std::is_same_v<T, U>,
        std::integral_constant<std::size_t, I>,
        find_index_impl<I + 1, T, Us...>
    >
{};

} // detail

template<class T, class List>
struct find_index;

template<class T, template<class...> class TT, class... Ts>
struct find_index<T, TT<Ts...>> : detail::find_index_impl<0, T, Ts...> {};

template<class T, class List>
inline constexpr std::size_t find_index_v = find_index<T, List>::value;


template<class T, class... Ts>
struct is_in : std::disjunction<std::is_same<T, Ts>...> {};

template<class T, class... Ts>
inline constexpr bool is_in_v = is_in<T, Ts...>::value;


template<template<class A, class B> class F, class U, class... Ts>
struct disjunction_for : std::disjunction<F<U, Ts>...> {};

template<template<class A, class B> class F, class U, class... Ts>
inline constexpr bool disjunction_for_v = disjunction_for<F, U, Ts...>::value;

template<template<class A, class B> class F, class U, class... Ts>
struct conjunction_for : std::conjunction<F<U, Ts>...> {};

template<template<class A, class B> class F, class U, class... Ts>
inline constexpr bool conjunction_for_v = conjunction_for<F, U, Ts...>::value;


namespace detail {

template<bool Found, class T, class... Us>
struct exactly_once_impl : std::bool_constant<Found> {};

template<class T, class U, class... Us>
struct exactly_once_impl<false, T, U, Us...>
    : exactly_once_impl<std::is_same_v<T, U>, T, Us...> {};

template<class T, class U, class... Us>
struct exactly_once_impl<true, T, U, Us...>
    : std::conditional_t<std::is_same_v<T, U>, std::false_type, exactly_once_impl<true, T, Us...>> {};

} // detail

template<class T, class List>
struct exactly_once;

template<class T, template<class...> class TT, class... Ts>
struct exactly_once<T, TT<Ts...>> : detail::exactly_once_impl<false, T, Ts...>
{
    static_assert(sizeof...(Ts) > 0);
};

template<class T, class List>
inline constexpr bool exactly_once_v = exactly_once<T, List>::value;


// Provides definition equivalent to MSVC's STL for semantic compatibility
namespace detail::has_ADL_swap_detail {

#if defined(__clang__) || defined(__EDG__)
void swap() = delete; // poison pill
#else
void swap();
#endif

template<class, class = void> struct has_ADL_swap : std::false_type {};
template<class T> struct has_ADL_swap<T, std::void_t<decltype(swap(std::declval<T&>(), std::declval<T&>()))>> : std::true_type {};

} // detail::has_ADL_swap_detail


template<class T>
struct is_trivially_swappable : std::conjunction<
    std::is_trivially_destructible<T>,
    std::is_trivially_move_constructible<T>,
    std::is_trivially_move_assignable<T>,
    // std::is_swappable cannot be used for this purpose because it assumes `using std::swap`
    std::negation<detail::has_ADL_swap_detail::has_ADL_swap<T>>
>
{};
template<> struct is_trivially_swappable<std::byte> : std::true_type {};

template<class T>
constexpr bool is_trivially_swappable_v = is_trivially_swappable<T>::value;


namespace detail {

template<std::size_t I, class Ti>
struct aggregate_initialize_tag
{
    static constexpr std::size_t index = I;
    using type = Ti;
};

// This version works better than MSVC's, does not break IntelliSense or ReSharper
template<std::size_t I, class Ti>
struct aggregate_initialize_overload
{
    using TiA = Ti[];

    // https://eel.is/c++draft/dcl.init.general#14
    // https://eel.is/c++draft/dcl.init.list#3.4
    // https://eel.is/c++draft/dcl.init.aggr#3

    template<class T>
    auto operator()(Ti, T&&) -> aggregate_initialize_tag<I, Ti>
        requires requires(T&& t) { { TiA{std::forward<T>(t)} }; } // emulate `Ti x[] = {std::forward<T>(t)};`
    {
        return {}; // silence MSVC warning
    }
};

template<class Is, class... Ts>
struct aggregate_initialize_fun;

// Imaginary function FUN of https://eel.is/c++draft/variant#ctor-14
template<std::size_t... Is, class... Ts>
struct aggregate_initialize_fun<std::index_sequence<Is...>, Ts...>
    : aggregate_initialize_overload<Is, Ts>...
{
    using aggregate_initialize_overload<Is, Ts>::operator()...;
};

template<class... Ts>
using aggregate_initialize_fun_for = aggregate_initialize_fun<std::index_sequence_for<Ts...>, Ts...>;

template<class Enabled, class T, class... Ts>
struct aggregate_initialize_resolution {};

template<class T, class... Ts>
struct aggregate_initialize_resolution<
    std::void_t<decltype(aggregate_initialize_fun_for<Ts...>{}(std::declval<T>(), std::declval<T>()))>, T, Ts...
> {
    using tag = decltype(aggregate_initialize_fun_for<Ts...>{}(std::declval<T>(), std::declval<T>()));
    using type = typename tag::type;
    static constexpr std::size_t index = tag::index;
};

} // detail

// We intentionally don't provide the convenient `_t` and `_v` aliases
// because they would lead to unnecessarily nested instantiation for
// legitimate infinite recursion errors on recursive types.
template<class T, class... Ts>
struct aggregate_initialize_resolution : detail::aggregate_initialize_resolution<void, T, Ts...> {};


// https://eel.is/c++draft/function.objects.general
template<class F, class... Args>
struct is_function_object : std::false_type {};

template<class F, class... Args>
constexpr bool is_function_object_v = is_function_object<F, Args...>::value;

template<class F, class... Args>
    requires
        (std::is_pointer_v<F> && std::is_function_v<std::remove_pointer_t<F>>) ||
        (std::is_class_v<F> && requires(F f) {
            { f(std::declval<Args>()...) };
        })
struct is_function_object<F, Args...> : std::true_type {};

} // yk::core

#endif
