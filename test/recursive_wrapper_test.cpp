// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "yk/rvariant/recursive_wrapper.hpp"
#include "yk/rvariant/rvariant.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <variant>
#include <type_traits>
#include <concepts>

namespace unit_test {

TEST_CASE("relational operators", "[wrapper]")
{
    {
        yk::recursive_wrapper<int> a(33), b(4);

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
        yk::recursive_wrapper<int> a(33);
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
        yk::recursive_wrapper<int> b(4);

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

        yk::recursive_wrapper<int> a(33);
        yk::recursive_wrapper<int, MyAllocator> b(4);

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

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4244) // implicit numeric conversion
#endif

TEST_CASE("implicit conversion (int/double)", "[wrapper]")
{
    struct NonExistent {};
    using RW_int = yk::recursive_wrapper<int>;
    using RW_double = yk::recursive_wrapper<double>;

    STATIC_REQUIRE( std::convertible_to<int, RW_int>); // implicit & explicit
    STATIC_REQUIRE(!std::convertible_to<RW_int, int>); // implicit & explicit
    STATIC_REQUIRE(!std::is_constructible_v<int, RW_int>);

    // ==============================================
    // Variations for LHS = int
    // ==============================================
    STATIC_REQUIRE(std::is_constructible_v<int, int>);    // recursive_wrapper should replicate this
    STATIC_REQUIRE(std::is_constructible_v<int, double>); // recursive_wrapper should replicate this

    STATIC_REQUIRE( std::is_constructible_v<RW_int, RW_int>);
    STATIC_REQUIRE( std::is_constructible_v<RW_int, int>);
    STATIC_REQUIRE( std::is_constructible_v<RW_int, double>);
    STATIC_REQUIRE(!std::is_constructible_v<RW_int, RW_double>); // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<RW_int, NonExistent>);

    // ----------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_int>, RW_int>);
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_int>, int>);
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_int>, double>);
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_int>, RW_double>); // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_int>, NonExistent>);
    // ----------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int>, RW_int>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int>, int>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int>, double>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int>, RW_double>); // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int>, NonExistent>);

    // ==============================================
    // Variations for LHS = double
    // ==============================================
    STATIC_REQUIRE(std::is_constructible_v<double, int>);    // recursive_wrapper should replicate this
    STATIC_REQUIRE(std::is_constructible_v<double, double>); // recursive_wrapper should replicate this

    STATIC_REQUIRE( std::is_constructible_v<RW_double, RW_double>);
    STATIC_REQUIRE( std::is_constructible_v<RW_double, int>);
    STATIC_REQUIRE( std::is_constructible_v<RW_double, double>);
    STATIC_REQUIRE(!std::is_constructible_v<RW_double, RW_int>); // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<RW_double, NonExistent>);

    // ----------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_double>, RW_double>);
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_double>, int>);
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_double>, double>);
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_double>, RW_int>); // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_double>, NonExistent>);
    // ----------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_double>, RW_double>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_double>, int>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_double>, double>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_double>, RW_int>); // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_double>, NonExistent>);


    // ----------------------------------------------

    // (int|double) = int
    STATIC_REQUIRE( std::is_constructible_v<std::variant<int, double>, int>);
    STATIC_REQUIRE(                         std::variant<int, double>{42}.index() == 0);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, double>, int>);
    STATIC_REQUIRE(                         yk::rvariant<int, double>{42}.index() == 0);

    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_int, double>, int>);
    STATIC_REQUIRE(                         std::variant<RW_int, double>{42}.index() == 0);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, double>, int>);
    STATIC_REQUIRE(                         yk::rvariant<RW_int, double>{42}.index() == 0);

    STATIC_REQUIRE( std::is_constructible_v<std::variant<int, RW_double>, int>);
    STATIC_REQUIRE(                         std::variant<int, RW_double>{42}.index() == 0);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, RW_double>, int>);
    STATIC_REQUIRE(                         yk::rvariant<int, RW_double>{42}.index() == 0);

    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_int, RW_double>, int>); // banned; equally viable
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, RW_double>, int>); // banned; equally viable

    // (int|double) = double
    STATIC_REQUIRE( std::is_constructible_v<std::variant<int, double>, double>);
    STATIC_REQUIRE(                         std::variant<int, double>{3.14}.index() == 1);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, double>, double>);
    STATIC_REQUIRE(                         yk::rvariant<int, double>{3.14}.index() == 1);

    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_int, double>, double>);
    STATIC_REQUIRE(                         std::variant<RW_int, double>{3.14}.index() == 1);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, double>, double>);
    STATIC_REQUIRE(                         yk::rvariant<RW_int, double>{3.14}.index() == 1);

    STATIC_REQUIRE( std::is_constructible_v<std::variant<int, RW_double>, double>);
    STATIC_REQUIRE(                         std::variant<int, RW_double>{3.14}.index() == 1);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, RW_double>, double>);
    STATIC_REQUIRE(                         yk::rvariant<int, RW_double>{3.14}.index() == 1);

    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_int, RW_double>, double>); // banned; equally viable
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, RW_double>, double>); // banned; equally viable

    // (int|double) = float
    STATIC_REQUIRE( std::is_constructible_v<std::variant<int, double>, float>);
    STATIC_REQUIRE(                         std::variant<int, double>{3.14f}.index() == 1);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, double>, float>);
    STATIC_REQUIRE(                         yk::rvariant<int, double>{3.14f}.index() == 1);

    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_int, double>, float>);
    STATIC_REQUIRE(                         std::variant<RW_int, double>{3.14f}.index() == 1);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, double>, float>);
    STATIC_REQUIRE(                         yk::rvariant<RW_int, double>{3.14f}.index() == 1);

    STATIC_REQUIRE( std::is_constructible_v<std::variant<int, RW_double>, float>);
    STATIC_REQUIRE(                         std::variant<int, RW_double>{3.14f}.index() == 1);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, RW_double>, float>);
    STATIC_REQUIRE(                         yk::rvariant<int, RW_double>{3.14f}.index() == 1);

    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_int, RW_double>, float>); // banned; equally viable
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, RW_double>, float>); // banned; equally viable
}

TEST_CASE("implicit conversion (string)", "[wrapper]")
{
    struct NonExistent {};
    using RW_string = yk::recursive_wrapper<std::string>;
    using RW_string_view = yk::recursive_wrapper<std::string_view>;

    STATIC_REQUIRE( std::convertible_to<std::string, RW_string>); // implicit & explicit
    STATIC_REQUIRE(!std::convertible_to<RW_string, std::string>); // implicit & explicit
    STATIC_REQUIRE(!std::is_constructible_v<std::string, RW_string>);

    // ==============================================
    // Variations for LHS = string
    // ==============================================
    STATIC_REQUIRE( std::is_constructible_v<std::string, std::string>);      // recursive_wrapper should replicate this
    STATIC_REQUIRE( std::is_constructible_v<std::string, std::string_view>); // recursive_wrapper does NOT replicate this

    STATIC_REQUIRE( std::is_constructible_v<RW_string, RW_string>);
    STATIC_REQUIRE( std::is_constructible_v<RW_string, std::string>);
    STATIC_REQUIRE( std::is_constructible_v<RW_string, char const*>);
    STATIC_REQUIRE(!std::is_constructible_v<RW_string, std::string_view>); // banned; requires explicit constructor
    STATIC_REQUIRE(!std::is_constructible_v<RW_string, RW_string_view>); // banned for Literal<String, StringView> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<RW_string, NonExistent>);

    STATIC_REQUIRE( std::is_constructible_v<RW_string_view, RW_string_view>);
    STATIC_REQUIRE( std::is_constructible_v<RW_string_view, std::string>);
    STATIC_REQUIRE( std::is_constructible_v<RW_string_view, char const*>);
    STATIC_REQUIRE( std::is_constructible_v<RW_string_view, std::string_view>);
    STATIC_REQUIRE(!std::is_constructible_v<RW_string_view, RW_string>); // banned for Literal<String, StringView> to be distinguishable in ASTs

    // ----------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<std::variant<std::string>, std::string>);      // recursive_wrapper should replicate this
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<std::string>, std::string_view>); // recursive_wrapper should replicate this
    // NOTE: below is ill-formed
    //{
    //    std::string x[] = {std::string_view{"foo"}}; // imaginary function
    //    std::variant<std::string> v(std::string_view{"foo"});
    //}

    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_string>, RW_string>);
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_string>, std::string>);
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_string>, char const*>);
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_string>, std::string_view>); // banned; requires explicit constructor
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_string>, NonExistent>);
    // ----------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_string>, RW_string>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_string>, std::string>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_string>, char const*>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_string>, std::string_view>); // banned; requires explicit constructor
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_string>, NonExistent>);
}

TEST_CASE("implicit conversion + flexible (int/double)", "[wrapper][flexible]")
{
    using RW_int = yk::recursive_wrapper<int>;
    using RW_double = yk::recursive_wrapper<double>;

    // ==============================================
    // Variations for LHS = int
    // ==============================================
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_int>, std::variant<RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_int>, std::variant<int>>);       // no flexible constructor
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_int>, std::variant<double>>);    // no flexible constructor
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_int>, std::variant<RW_double>>); // no flexible constructor
    // ----------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int>, yk::rvariant<int>>);       // flexible constructor
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int>, yk::rvariant<double>>);    // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int>, yk::rvariant<RW_double>>); // banned for Literal<int, double> to be distinguishable in ASTs

    // ==============================================
    // Variations for LHS = double
    // ==============================================
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_double>, std::variant<RW_int>>);    // no flexible constructor
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_double>, std::variant<int>>);       // no flexible constructor
    STATIC_REQUIRE(!std::is_constructible_v<std::variant<RW_double>, std::variant<double>>);    // no flexible constructor
    STATIC_REQUIRE( std::is_constructible_v<std::variant<RW_double>, std::variant<RW_double>>);
    // ----------------------------------------------
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_double>, yk::rvariant<RW_int>>);    // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_double>, yk::rvariant<int>>);       // banned for Literal<int, double> to be distinguishable in ASTs
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_double>, yk::rvariant<double>>);    // flexible constructor
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_double>, yk::rvariant<RW_double>>);

    // TODO: more tests
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

} // unit_test
