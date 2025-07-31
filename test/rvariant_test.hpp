#ifndef YK_RVARIANT_TEST_HPP
#define YK_RVARIANT_TEST_HPP

#include "yk/rvariant/rvariant.hpp"

#include <iosfwd>
#include <compare>
#include <functional>
#include <exception>
#include <utility>

namespace unit_test {

namespace MC_Thrower_ADL_guard {

struct MC_Thrower
{
    struct non_throwing_t {};
    struct throwing_t {};
    struct potentially_throwing_t {};

    static constexpr non_throwing_t non_throwing{};
    static constexpr throwing_t throwing{};
    static constexpr potentially_throwing_t potentially_throwing{};

    MC_Thrower() = default;
    MC_Thrower(MC_Thrower const&) = default;
    MC_Thrower(MC_Thrower&&) noexcept(false) { throw std::exception{}; }
    MC_Thrower& operator=(MC_Thrower const&) = default;
    MC_Thrower& operator=(MC_Thrower&&) = default;

    MC_Thrower(non_throwing_t) noexcept {}
    MC_Thrower(throwing_t) noexcept(false) { throw std::exception{}; }
    MC_Thrower(potentially_throwing_t) noexcept(false) {}

    friend bool operator==(MC_Thrower const&, MC_Thrower const&) noexcept { return true; }
    friend auto operator<=>(MC_Thrower const&, MC_Thrower const&) noexcept { return std::strong_ordering::equal; }
};

std::ostream& operator<<(std::ostream&, MC_Thrower const&);

} // MC_Thrower_ADL_guard

using MC_Thrower_ADL_guard::MC_Thrower;

template <class... Ts, class... Args>
[[nodiscard]] constexpr yk::rvariant<Ts..., MC_Thrower> make_valueless(Args&&... args)
{
    using V = yk::rvariant<Ts..., MC_Thrower>;

    static_assert(std::is_nothrow_constructible_v<V, Args...>);
    V a(std::forward<Args>(args)...);

    try {
        V b(std::in_place_type<MC_Thrower>);
        a = std::move(b);
    } catch(...) {  // NOLINT(bugprone-empty-catch)
    }
    return a;
}

template<class T>
struct HashForwarded
{
    T value;
};

}  // unit_test


namespace std {

template<class T>
struct hash<::unit_test::HashForwarded<T>>
{
    size_t operator()(::unit_test::HashForwarded<T> const& v) const
    {
        return std::hash<T>{}(v.value);
    }
};

} // std

#endif
