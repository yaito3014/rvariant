#ifndef YK_DETAIL_FIND_INDEX_HPP
#define YK_DETAIL_FIND_INDEX_HPP

#include "yk/detail/is_in.hpp"

#include <type_traits>

#include <cstddef>

namespace yk::detail {

inline constexpr std::size_t find_index_npos = -1;

template<std::size_t I, class T, class... Ts>
struct find_index_impl : std::integral_constant<std::size_t, find_index_npos> {};

template<std::size_t I, class T, class U, class... Us>
struct find_index_impl<I, T, U, Us...> : std::conditional_t<std::is_same_v<T, U>, std::integral_constant<std::size_t, I>, find_index_impl<I + 1, T, Us...>> {};

template<class T, class List>
struct find_index;

template<class T, template<class...> class TT, class... Ts>
struct find_index<T, TT<Ts...>> : find_index_impl<0, T, Ts...> {};

template<class T, class List>
inline constexpr std::size_t find_index_v = find_index<T, List>::value;

}  // namespace yk::detail

#endif  // YK_DETAIL_FIND_INDEX_HPP
