#include "yk/indirect.hpp"

#include <catch2/catch_test_macros.hpp>


TEST_CASE("indirect")
{
    yk::indirect<int> a(42);
    yk::indirect<int> b = a;             // copy ctor
    yk::indirect<int> c = std::move(a);  // move ctor
    c = b;                               // copy assign
    c = std::move(b);                    // move assign

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 26800) // use-after-free
#endif

    REQUIRE(a.valueless_after_move());
    REQUIRE(b.valueless_after_move());

#ifdef _MSC_VER
# pragma warning(pop)
#endif

    REQUIRE_FALSE(c.valueless_after_move());
}
