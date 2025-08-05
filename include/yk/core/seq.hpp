#ifndef YK_CORE_SEQ_HPP
#define YK_CORE_SEQ_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <type_traits>
#include <utility>

namespace yk::core {

template<class... Ts>
struct type_list;


namespace detail {

template<class S, template<auto...> class F>
struct seq_rebind_impl;

// rebind<type_list<std::integral_constant<std::size_t, 0>>, std::index_sequence>
template<template<class...> class TT, class... Ts, template<auto...> class F>
struct seq_rebind_impl<TT<Ts...>, F>
{
    using type = F<Ts::value...>;
};

} // detail

template<class S, template<auto...> class F>
using seq_rebind = typename detail::seq_rebind_impl<S, F>::type;


namespace detail {

template<class... Ss>
struct seq_concat_impl;

template<class... Ts>
struct seq_concat_impl<type_list<Ts...>>
{
    using type = type_list<Ts...>;
};

// concat<type_list<...>, type_list<...>>
template<class... Ts, class... Us, class... Rest>
struct seq_concat_impl<type_list<Ts...>, type_list<Us...>, Rest...>
    : seq_concat_impl<type_list<Ts..., Us...>, Rest...>
{};

} // detail

template<class... Ss>
using seq_concat = typename detail::seq_concat_impl<Ss...>::type;


namespace detail {

template<class S, class T>
struct seq_push_back_impl;

// push_back<type_list<>, std::integral_constant<std::size_t, 0>>
template<class T, template<class...> class TT, class... Ts>
struct seq_push_back_impl<TT<Ts...>, T>
{
    using type = TT<Ts..., T>;
};

} // detail

template<class S, class T>
using seq_push_back = typename detail::seq_push_back_impl<S, T>::type;


namespace detail {

template<class T, class U>
struct seq_assign_impl;

// assign<type_list<>, type_list<...>>
template<template<class...> class TT, class... Ts, template<class...> class UU, class... Us>
struct seq_assign_impl<TT<Ts...>, UU<Us...>>
{
    using type = TT<Us...>;
};

} // detail

template<class T, class U>
using seq_assign = typename detail::seq_assign_impl<T, U>::type;


namespace detail {

template<template<auto...> class F, class S, class... Rest>
struct seq_cartesian_product_impl;

// impl2<std::index_sequence, type_list<std::integral_constant<std::size_t, 0>>
template<template<auto...> class F, class S>
struct seq_cartesian_product_impl<F, S>
{
    using type = type_list<seq_rebind<S, F>>;
};

// impl2<std::index_sequence, type_list<>, std::index_sequence<0>>
template<template<auto...> class F, class S, class T, auto... As, class... Rest>
struct seq_cartesian_product_impl<F, S, std::integer_sequence<T, As...>, Rest...>
{
    using type = seq_concat<
        typename seq_cartesian_product_impl<
            F,
            seq_push_back<S, std::integral_constant<T, As>>,
            Rest...
        >::type...
    >;
};

template<template<auto...> class F, class... Ss>
struct seq_cartesian_product_entry;

// impl<std::index_sequence, std::index_sequence<0>>
template<template<auto...> class F, class T, auto... As, class... Rest>
struct seq_cartesian_product_entry<F, std::integer_sequence<T, As...>, Rest...>
{
    using type = seq_assign<
        type_list<>,
        typename seq_cartesian_product_impl<
            F,
            type_list<>,
            std::integer_sequence<T, As...>,
            Rest...
        >::type
    >;
};

} // detail

template<template<auto...> class F, class... Ss>
using seq_cartesian_product = typename detail::seq_cartesian_product_entry<F, Ss...>::type;

} // yk::core

#endif
