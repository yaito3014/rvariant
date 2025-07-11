#ifndef YK_DETAIL_SUBSET_LIKE_HPP
#define YK_DETAIL_SUBSET_LIKE_HPP

#include "yk/detail/is_in.hpp"

#include <type_traits>

namespace yk::detail {

template<class T, class U>
struct subset_like : std::false_type {};

template<template<class...> class L1, class... Ts, template<class...> class L2, class... Us>
struct subset_like<L1<Ts...>, L2<Us...>> : std::conjunction<is_in<Us, Ts...>...> {};

template<class T, class U>
inline constexpr bool subset_like_v = subset_like<T, U>::value;

}  // namespace yk::detail

#endif  // YK_DETAIL_SUBSET_LIKE_HPP
