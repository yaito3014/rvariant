#ifndef YK_DETAIL_TYPE_SET_HPP
#define YK_DETAIL_TYPE_SET_HPP

#include <type_traits>

namespace yk::detail {

template <class T, class... Ts>
struct is_in : std::disjunction<std::is_same<T, Ts>...> {};

template <class T, class... Ts>
inline constexpr bool is_in_v = is_in<T, Ts...>::value;

template <class... Ts>
struct is_unique;

template <class... Ts>
inline constexpr bool is_unique_v = is_unique<Ts...>::value;

template <>
struct is_unique<> : std::true_type {};

template <class T, class... Ts>
struct is_unique<T, Ts...> : std::conjunction<std::negation<is_in<T, Ts...>>, is_unique<Ts...>> {};

}  // namespace yk::detail

#endif  // YK_DETAIL_TYPE_SET_HPP
