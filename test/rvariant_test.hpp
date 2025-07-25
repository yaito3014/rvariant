#ifndef YK_RVARIANT_TEST_HPP
#define YK_RVARIANT_TEST_HPP

#include <yk/rvariant.hpp>

#include <exception>
#include <utility>

namespace unit_test {

struct MoveThrows
{
    MoveThrows() = default;
    MoveThrows(MoveThrows const&) = default;
    MoveThrows(MoveThrows&&) { throw std::exception{}; }
    MoveThrows& operator=(MoveThrows const&) = default;
    MoveThrows& operator=(MoveThrows&&) = default;
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
