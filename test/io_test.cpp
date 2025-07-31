#include <iostream>

namespace {

namespace NonStreamable_ns {

// Not streamable via ADL, but...
struct NonStreamable {};

} // NonStreamable_ns

// Bad global overload; can be avoided by the poison pill
[[maybe_unused]] std::ostream& operator<<(std::ostream& os, NonStreamable_ns::NonStreamable&)
{
    return os << "polluted &";
}

// Bad global overload; can be avoided by the poison pill
[[maybe_unused]] std::ostream& operator<<(std::ostream& os, NonStreamable_ns::NonStreamable const&)
{
    return os << "polluted const&";
}

// Bad global overload; can be avoided by the poison pill
[[maybe_unused]] std::ostream& operator<<(std::ostream& os, [[maybe_unused]] NonStreamable_ns::NonStreamable&&)
{
    return os << "polluted &&";
}

// Bad global overload; can be avoided by the poison pill
[[maybe_unused]] std::ostream& operator<<(std::ostream& os, NonStreamable_ns::NonStreamable const&&)
{
    return os << "polluted const&&";
}

} // anonymous global

#include "yk/core/io.hpp" // this finds `operator<<` in the global ns

#include "rvariant_test.hpp"

#include "yk/rvariant/rvariant.hpp"
#include "yk/rvariant/rvariant_io.hpp"
#include "yk/rvariant/recursive_wrapper.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <sstream>


namespace unit_test {

namespace MC_Thrower_ADL_guard {

std::ostream& operator<<(std::ostream& os, MC_Thrower const&)
{
    return os << "MC_Thrower";
}

} // MC_Thrower_ADL_guard


namespace {

namespace S_ns {

struct S
{
    std::string msg;
};

std::ostream& operator<<(std::ostream& os, S const& s)
{
    return os << s.msg;
}

} // S_ns

namespace ThrowingValue_ns {

struct ThrowingValue {};

// Not derived from std::exception
struct StrangeException {};

std::ostream& operator<<(std::ostream& os, ThrowingValue const&)
{
    os << "ThrowingValue";
    throw StrangeException{};  // NOLINT(hicpp-exception-baseclass)
}

} // ThrowingValue_ns

} // anonymous


TEST_CASE("rvariant.io, simple")
{
    using S_ns::S;

    {
        yk::rvariant<int> v{42};
        std::ostringstream oss;
        oss << v;
        CHECK(oss.str() == "42");
    }
    {
        STATIC_REQUIRE(yk::core::ADL_ostreamable_v<S>);
        STATIC_REQUIRE(yk::core::ADL_ostreamable_v<yk::rvariant<S>>);
        yk::rvariant<S> v{S{"foo"}};
        std::ostringstream oss;
        oss << v;
        CHECK(oss.str() == "foo");
    }
    {
        struct NonExistent;
        STATIC_REQUIRE(!yk::core::ADL_ostreamable_v<NonExistent>);
        struct NonExistent {};
        STATIC_REQUIRE(!yk::core::ADL_ostreamable_v<yk::rvariant<NonExistent>>);
    }
    {
        STATIC_REQUIRE(!yk::core::ADL_ostreamable_v<NonStreamable_ns::NonStreamable>);
        STATIC_REQUIRE(!yk::core::ADL_ostreamable_v<yk::rvariant<NonStreamable_ns::NonStreamable>>);

        std::ostringstream oss;
        NonStreamable_ns::NonStreamable const non_streamable;
        oss << non_streamable;
        CHECK(oss.str() == "polluted const&");
    }

    // alternative = int
    {
        yk::rvariant<int, S> v{42};
        std::ostringstream oss;
        oss << v;
        CHECK(oss.str() == "42");
    }
    // alternative = int
    {
        yk::rvariant<S, int> v{42};
        std::ostringstream oss;
        oss << v;
        CHECK(oss.str() == "42");
    }

    // alternative = S
    {
        yk::rvariant<int, S> v{S{"foo"}};
        std::ostringstream oss;
        oss << v;
        CHECK(oss.str() == "foo");
    }
    // alternative = S
    {
        yk::rvariant<S, int> v{S{"foo"}};
        std::ostringstream oss;
        oss << v;
        CHECK(oss.str() == "foo");
    }
}

TEST_CASE("rvariant.io, exceptions")
{
    using ThrowingValue_ns::ThrowingValue;

    // good at first + no exceptions
    {
        yk::rvariant<ThrowingValue> v;
        std::ostringstream oss; // good at first
        REQUIRE_NOTHROW(oss << v);
        CHECK(oss.str() == "ThrowingValue");
        CHECK((oss.rdstate() & std::ios_base::badbit) != 0);
    }
    // bad at first + no exceptions
    {
        yk::rvariant<ThrowingValue> v;
        std::ostringstream oss;
        oss.setstate(std::ios_base::badbit); // BAD at first
        REQUIRE_NOTHROW(oss << v);
        CHECK(oss.str().empty());
        CHECK((oss.rdstate() & std::ios_base::badbit) != 0);
    }

    // good at first + exceptions
    {
        yk::rvariant<ThrowingValue> v;
        std::ostringstream oss; // good at first
        oss.exceptions(std::ios_base::badbit);
        REQUIRE_THROWS_AS(oss << v, ThrowingValue_ns::StrangeException);
        CHECK(oss.str() == "ThrowingValue");
        CHECK((oss.rdstate() & std::ios_base::badbit) != 0);
    }
    // bad at first + exceptions
    {
        yk::rvariant<ThrowingValue> v;
        std::ostringstream oss;
        oss.setstate(std::ios_base::badbit); // BAD at first
        try {
            oss.exceptions(std::ios_base::badbit);
        } catch (std::ios_base::failure const&) {  // NOLINT(bugprone-empty-catch)
            // propagate invalid stream
        }
        REQUIRE_THROWS_AS(oss << v, std::ios_base::failure);
        CHECK(oss.str().empty());
        CHECK((oss.rdstate() & std::ios_base::badbit) != 0);
    }

    // [valueless] good at first + no exceptions
    {
        yk::rvariant<int, MC_Thrower> v = make_valueless<int>();
        std::ostringstream oss; // good at first
        REQUIRE_THROWS_AS(oss << v, std::bad_variant_access); // valueless always throws
        CHECK(oss.str().empty());
    }
    // [valueless] bad at first + no exceptions
    {
        yk::rvariant<int, MC_Thrower> v = make_valueless<int>();
        std::ostringstream oss;
        oss.setstate(std::ios_base::badbit); // BAD at first
        REQUIRE_NOTHROW(oss << v); // sentry should be engaged
        CHECK(oss.str().empty());
        CHECK((oss.rdstate() & std::ios_base::badbit) != 0); // still BAD
    }

    // [valueless] good at first + exceptions
    {
        yk::rvariant<int, MC_Thrower> v = make_valueless<int>();
        std::ostringstream oss; // good at first
        oss.exceptions(std::ios_base::badbit);
        REQUIRE_THROWS_AS(oss << v, std::bad_variant_access); // valueless always throws
        CHECK(oss.str().empty());
    }
    // [valueless] bad at first + exceptions
    {
        yk::rvariant<int, MC_Thrower> v = make_valueless<int>();
        std::ostringstream oss;
        oss.setstate(std::ios_base::badbit); // BAD at first
        try {
            oss.exceptions(std::ios_base::badbit);
        } catch (std::ios_base::failure const&) {  // NOLINT(bugprone-empty-catch)
            // propagate invalid stream
        }
        REQUIRE_THROWS_AS(oss << v, std::ios_base::failure); // should do nothing
        CHECK(oss.str().empty());
        CHECK((oss.rdstate() & std::ios_base::badbit) != 0); // still BAD
    }

    // [nested valueless] good at first + no exceptions
    {
        yk::rvariant<int, yk::rvariant<int, MC_Thrower>> v = make_valueless<int>();
        std::ostringstream oss; // good at first
        REQUIRE_THROWS_AS(oss << v, std::bad_variant_access); // valueless always throws
        CHECK(oss.str().empty());
    }
    // [nested valueless] bad at first + no exceptions
    {
        yk::rvariant<int, yk::rvariant<int, MC_Thrower>> v = make_valueless<int>();
        std::ostringstream oss;
        oss.setstate(std::ios_base::badbit); // BAD at first
        REQUIRE_NOTHROW(oss << v);
        CHECK(oss.str().empty());
        CHECK((oss.rdstate() & std::ios_base::badbit) != 0); // still BAD
    }

    // [nested valueless] good at first + exceptions
    {
        yk::rvariant<int, yk::rvariant<int, MC_Thrower>> v = make_valueless<int>();
        std::ostringstream oss; // good at first
        oss.exceptions(std::ios_base::badbit);
        REQUIRE_THROWS_AS(oss << v, std::bad_variant_access); // valueless always throws
        CHECK(oss.str().empty());
    }
    // [nested valueless] bad at first + exceptions
    {
        yk::rvariant<int, yk::rvariant<int, MC_Thrower>> v = make_valueless<int>();
        std::ostringstream oss;
        oss.setstate(std::ios_base::badbit); // BAD at first
        try {
            oss.exceptions(std::ios_base::badbit);
        } catch (std::ios_base::failure const&) {  // NOLINT(bugprone-empty-catch)
            // propagate invalid stream
        }
        REQUIRE_THROWS_AS(oss << v, std::ios_base::failure); // should do nothing
        CHECK(oss.str().empty());
        CHECK((oss.rdstate() & std::ios_base::badbit) != 0); // still BAD
    }
}

TEST_CASE("rvariant.io", "[recursive]")
{
    {
        yk::rvariant<yk::recursive_wrapper<int>> v(42);
        std::ostringstream oss;
        oss << v;
        CHECK(oss.str() == "42");
    }
}

} // unit_test
