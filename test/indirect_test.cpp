// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "yk/indirect.hpp"

#include <catch2/catch_test_macros.hpp>

namespace unit_test {

TEST_CASE("construction and assignment", "[indirect]")
{
    yk::indirect<int> a(42);
    yk::indirect<int> b = a;             // copy ctor
    yk::indirect<int> c = std::move(a);  // move ctor
    c = b;                               // copy assign
    (void)c;
    c = std::move(b);                    // move assign

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 26800) // use-after-free
#endif

    REQUIRE(a.valueless_after_move());  // NOLINT(bugprone-use-after-move)
    REQUIRE(b.valueless_after_move());  // NOLINT(bugprone-use-after-move)

#ifdef _MSC_VER
# pragma warning(pop)
#endif

    REQUIRE_FALSE(c.valueless_after_move());
}

TEST_CASE("construction and assignment (stateful allocator)", "[indirect]")
{
    struct MyAllocator : std::allocator<int>
    {
        using is_always_equal = std::false_type;
    };
    yk::indirect<int, MyAllocator> a(42);
    yk::indirect<int, MyAllocator> b = a;             // copy ctor
    yk::indirect<int, MyAllocator> c = std::move(a);  // move ctor
    c = b;                               // copy assign
    (void)c;
    c = std::move(b);                    // move assign

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 26800) // use-after-free
#endif

    REQUIRE(a.valueless_after_move());  // NOLINT(bugprone-use-after-move)
    REQUIRE(b.valueless_after_move());  // NOLINT(bugprone-use-after-move)

#ifdef _MSC_VER
# pragma warning(pop)
#endif

    REQUIRE_FALSE(c.valueless_after_move());
}

TEST_CASE("dereference", "[indirect]")
{
    yk::indirect<int> a(42);

    CHECK(*a == 42);
    STATIC_CHECK(std::is_same_v<decltype(*a), int&>);

    CHECK(*std::as_const(a) == 42);
    STATIC_CHECK(std::is_same_v<decltype(*std::as_const(a)), int const&>);

    CHECK(*std::move(a) == 42);
    STATIC_CHECK(std::is_same_v<decltype(*std::move(a)), int&&>);

    CHECK(*std::move(std::as_const(a)) == 42);
    STATIC_CHECK(std::is_same_v<decltype(*std::move(std::as_const(a))), int const&&>);
}

TEST_CASE("relational operators", "[indirect]")
{
    {
        yk::indirect<int> a(33), b(4);

        CHECK(a == a);
        CHECK(a != b);
        CHECK(b < a);
        CHECK(a > b);
        CHECK(b <= a);
        CHECK(a >= b);

        CHECK((a <=> a) == std::strong_ordering::equal);
        CHECK((b <=> a) == std::strong_ordering::less);
        CHECK((a <=> b) == std::strong_ordering::greater);
    }
    {
        yk::indirect<int> a(33);
        int b = 4;

        CHECK(a == a);
        CHECK(a != b);
        CHECK(b < a);
        CHECK(a > b);
        CHECK(b <= a);
        CHECK(a >= b);

        CHECK((a <=> a) == std::strong_ordering::equal);
        CHECK((b <=> a) == std::strong_ordering::less);
        CHECK((a <=> b) == std::strong_ordering::greater);
    }
    {
        int a = 33;
        yk::indirect<int> b(4);

        CHECK(a == a);
        CHECK(a != b);
        CHECK(b < a);
        CHECK(a > b);
        CHECK(b <= a);
        CHECK(a >= b);

        CHECK((a <=> a) == std::strong_ordering::equal);
        CHECK((b <=> a) == std::strong_ordering::less);
        CHECK((a <=> b) == std::strong_ordering::greater);
    }
    {
        struct MyAllocator : std::allocator<int> {};

        yk::indirect<int> a(33);
        yk::indirect<int, MyAllocator> b(4);

        CHECK(a == a);
        CHECK(a != b);
        CHECK(b < a);
        CHECK(a > b);
        CHECK(b <= a);
        CHECK(a >= b);

        CHECK((a <=> a) == std::strong_ordering::equal);
        CHECK((b <=> a) == std::strong_ordering::less);
        CHECK((a <=> b) == std::strong_ordering::greater);
    }
}

} // unit_test
