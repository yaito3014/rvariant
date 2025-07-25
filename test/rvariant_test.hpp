#ifndef YK_RVARIANT_TEST_HPP
#define YK_RVARIANT_TEST_HPP

#include <yk/rvariant.hpp>

namespace unit_test {

struct MoveThrows {
    MoveThrows() = default;
    MoveThrows(MoveThrows const&) = default;
    MoveThrows(MoveThrows&&) { throw std::exception{}; }
    MoveThrows& operator=(MoveThrows const&) = default;
    MoveThrows& operator=(MoveThrows&&) = default;
};

yk::rvariant<int, MoveThrows> get_valueless() {
    yk::rvariant<int, MoveThrows> a;
    try {
        yk::rvariant<int, MoveThrows> b(std::in_place_index<1>);
        a = std::move(b);
    } catch(...) {
    }
    return a;
}

}  // unit_test

#endif // YK_RVARIANT_TEST_HPP
