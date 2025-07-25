#ifndef YK_RVARIANT_TEST_HPP
#define YK_RVARIANT_TEST_HPP

#include <yk/rvariant.hpp>

#include <exception>
#include <utility>

namespace unit_test {

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
    MoveThrows(MoveThrows&&) { throw std::exception{}; }
    MoveThrows& operator=(MoveThrows const&) = default;
    MoveThrows& operator=(MoveThrows&&) = default;

    MoveThrows(non_throwing_t) noexcept {}
    MoveThrows(throwing_t) noexcept(false) { throw std::exception{}; }
    MoveThrows(potentially_throwing_t) noexcept(false) {}
};

template <class... Ts, class... Args>
[[nodiscard]] yk::rvariant<Ts..., MoveThrows> make_valueless(Args&&... args)
{
    yk::rvariant<Ts..., MoveThrows> a(std::forward<Args>(args)...);
    try {
        yk::rvariant<Ts..., MoveThrows> b(std::in_place_type<MoveThrows>);
        a = std::move(b);
    } catch(...) {
    }
    return a;
}

}  // unit_test

#endif // YK_RVARIANT_TEST_HPP
