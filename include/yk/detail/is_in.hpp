#ifndef YK_DETAIL_IS_IN_HPP
#define YK_DETAIL_IS_IN_HPP

#include <type_traits>

namespace yk::detail {

template<class T, class... Ts>
struct is_in : std::disjunction<std::is_same<T, Ts>...> {};

template<class T, class... Ts>
inline constexpr bool is_in_v = is_in<T, Ts...>::value;

}  // namespace yk::detail

#endif  // YK_DETAIL_IS_IN_HPP
