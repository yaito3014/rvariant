#ifndef YK_RVARIANT_TEST_HPP
#define YK_RVARIANT_TEST_HPP

#include "yk/rvariant/rvariant.hpp"

#include <iosfwd>
#include <compare>
#include <exception>
#include <utility>

namespace unit_test {

namespace MoveThrows_ADL_guard {

// TODO: rename this to `MC_Thrower`
struct MoveThrows
{
    struct non_throwing_t {};
    struct throwing_t {};
    struct potentially_throwing_t {};

    static constexpr non_throwing_t non_throwing{};
    static constexpr throwing_t throwing{};
    static constexpr potentially_throwing_t potentially_throwing{};

    MoveThrows() = default;
    MoveThrows(MoveThrows const&) = default;
    MoveThrows(MoveThrows&&) noexcept(false) { throw std::exception{}; }
    MoveThrows& operator=(MoveThrows const&) = default;
    MoveThrows& operator=(MoveThrows&&) = default;

    MoveThrows(non_throwing_t) noexcept {}
    MoveThrows(throwing_t) noexcept(false) { throw std::exception{}; }
    MoveThrows(potentially_throwing_t) noexcept(false) {}

    friend bool operator==(MoveThrows const&, MoveThrows const&) noexcept { return true; }
    friend auto operator<=>(MoveThrows const&, MoveThrows const&) noexcept { return std::strong_ordering::equal; }
};

std::ostream& operator<<(std::ostream&, MoveThrows const&);

} // MoveThrows_ADL_guard

using MoveThrows_ADL_guard::MoveThrows;

template <class... Ts, class... Args>
[[nodiscard]] constexpr yk::rvariant<Ts..., MoveThrows> make_valueless(Args&&... args)
{
    using V = yk::rvariant<Ts..., MoveThrows>;

    static_assert(std::is_nothrow_constructible_v<V, Args...>);
    V a(std::forward<Args>(args)...);

    try {
        V b(std::in_place_type<MoveThrows>);
        a = std::move(b);
    } catch(...) {  // NOLINT(bugprone-empty-catch)
    }
    return a;
}

}  // unit_test

#endif // YK_RVARIANT_TEST_HPP
