#ifndef YK_DETAIL_FIND_INDEX_HPP
#define YK_DETAIL_FIND_INDEX_HPP

#include "yk/detail/is_in.hpp"

#include <type_traits>

#include <cstddef>

namespace yk::detail {

template<std::size_t I, class T, class... Ts>
struct find_index_impl;

template<std::size_t I, class T, class U, class... Us>
struct find_index_impl<I, T, U, Us...> : std::conditional_t<std::is_same_v<T, U>, std::integral_constant<std::size_t, I>, find_index_impl<I + 1, T, Us...>> {};

inline constexpr std::size_t find_index_npos = -1;

template<class T, class... Ts>
struct find_index : std::integral_constant<std::size_t, find_index_npos> {};

template<class T, class... Ts>
    requires is_in_v<T, Ts...>
struct find_index<T, Ts...> : find_index_impl<0, T, Ts...> {};

template<class T, class... Ts>
inline constexpr std::size_t find_index_v = find_index<T, Ts...>::value;

}  // namespace yk::detail

#endif  // YK_DETAIL_FIND_INDEX_HPP
