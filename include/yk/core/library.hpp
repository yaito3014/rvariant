#ifndef YK_CORE_LIBRARY_HPP
#define YK_CORE_LIBRARY_HPP

#include <yk/core/concepts.hpp>

#include <compare>
#include <functional>
#include <type_traits>

// Utilities defined in [library]
// https://eel.is/c++draft/library
namespace yk::core {
  
template<class T, class U = T>
inline constexpr bool synth_three_way_noexcept =
    std::conditional_t<
        std::three_way_comparable_with<T, U>,
        std::is_nothrow_invocable<std::compare_three_way, T const&, U const&>,
        std::conjunction<
          std::is_nothrow_invocable<std::less<>, T const&, U const&>,
          std::is_nothrow_invocable<std::less<>, U const&, T const&>
        >
    >::value;

constexpr auto synth_three_way = []<class T, class U>(const T& t, const U& u) noexcept(synth_three_way_noexcept<T, U>)
    -> std::conditional_t<std::three_way_comparable<T, U>, std::invoke_result_t<std::compare_three_way, T const&, U const&>, std::weak_ordering>
    requires requires {
        { t < u } -> core::boolean_testable;
        { u < t } -> core::boolean_testable;
    }
{
    if constexpr (std::three_way_comparable_with<T, U>) {
        return t <=> u;
    } else {
        if (t < u) return std::weak_ordering::less;
        if (u < t) return std::weak_ordering::greater;
        return std::weak_ordering::equivalent;
    }
};

template<class T, class U = T>
using synth_three_way_result = decltype(synth_three_way(std::declval<T&>(), std::declval<U&>()));

}  // namespace yk::core

#endif  // YK_CORE_LIBRARY_HPP
