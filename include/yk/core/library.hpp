#ifndef YK_CORE_LIBRARY_HPP
#define YK_CORE_LIBRARY_HPP

#include <yk/core/concepts.hpp>

#include <compare>
#include <functional>
#include <type_traits>

// Utilities defined in [library]
// https://eel.is/c++draft/library
namespace yk::core {

namespace detail {

template<class T, class U>
struct synth_three_way_result_impl
{
    using type = std::weak_ordering;
};

template<class T, class U> requires std::three_way_comparable<T, U>
struct synth_three_way_result_impl<T, U>
{
    using type = std::invoke_result_t<std::compare_three_way, T const&, U const&>;
};

} // detail

template<class T, class U = T>
using synth_three_way_result_t = typename detail::synth_three_way_result_impl<T, U>::type;

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

constexpr auto synth_three_way = []<class T, class U>(T const& t, U const& u) noexcept(synth_three_way_noexcept<T, U>)
    -> synth_three_way_result_t<T, U>
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

}  // namespace yk::core

#endif  // YK_CORE_LIBRARY_HPP
