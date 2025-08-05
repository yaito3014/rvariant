#ifndef YK_RVARIANT_PACK_HPP
#define YK_RVARIANT_PACK_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Utilities related to [rvariant.pack]

#include <yk/core/type_traits.hpp>

namespace yk {

namespace detail {

template<template<class...> class TT, class T, class... Us>
struct pack_union_impl;

template<template<class...> class TT, class... Ts>
struct pack_union_impl<TT, core::type_list<Ts...>>
{
    using type = TT<Ts...>;
};

template<template<class...> class TT, class... Ts, class U, class... Us>
struct pack_union_impl<TT, core::type_list<Ts...>, U, Us...>
    : std::conditional_t<
    core::is_in_v<U, Ts...>,
        pack_union_impl<TT, core::type_list<Ts...>, Us...>,
        pack_union_impl<TT, core::type_list<Ts..., U>, Us...>
    >
{};

template<template<class...> class TT, class A, class B>
struct pack_union;

template<template<class...> class TT, class A, class B>
struct pack_union : detail::pack_union_impl<TT, core::type_list<>, A, B> {};

template<template<class...> class TT, class... As, class B>
struct pack_union<TT, TT<As...>, B> : detail::pack_union_impl<TT, core::type_list<>, As..., B> {};

template<template<class...> class TT, class A, class... Bs>
struct pack_union<TT, A, TT<Bs...>> : detail::pack_union_impl<TT, core::type_list<>, A, Bs...> {};

template<template<class...> class TT, class... As, class... Bs>
struct pack_union<TT, TT<As...>, TT<Bs...>> : detail::pack_union_impl<TT, core::type_list<>, As..., Bs...> {};

template<template<class...> class TT, class A, class B>
using pack_union_t = typename pack_union<TT, A, B>::type;


template<class T>
struct unwrap_one_pack { using type = T; };

template<template<class...> class TT, class T>
struct unwrap_one_pack<TT<T>> { using type = T; };

} // detail


template<template<class...> class TT, class A, class B>
struct compact_alternative : detail::unwrap_one_pack<detail::pack_union_t<TT, A, B>> {};

template<template<class...> class TT, class A, class B>
using compact_alternative_t = typename compact_alternative<TT, A, B>::type;

} // yk

#endif
