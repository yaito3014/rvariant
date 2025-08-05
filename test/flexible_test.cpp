// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "rvariant_test.hpp"

#include "yk/rvariant.hpp"

#include <catch2/catch_test_macros.hpp>

namespace unit_test {

namespace {

template<class A, class B, class Expected>
constexpr bool is_same_alt = std::is_same_v<yk::compact_alternative_t<yk::rvariant, A, B>, Expected>;

} // anonymous

TEST_CASE("alternative", "[flexible]")
{
    STATIC_REQUIRE(is_same_alt<yk::rvariant<int>, yk::rvariant<int>, int>);
    STATIC_REQUIRE(is_same_alt<yk::rvariant<int>, yk::rvariant<double>, yk::rvariant<int, double>>);

    STATIC_REQUIRE(is_same_alt<yk::rvariant<int, double>, yk::rvariant<char, wchar_t>, yk::rvariant<int, double, char, wchar_t>>);
}

TEST_CASE("flexible", "[flexible]")
{
    struct X {};
    struct NonExistent {};
    // ------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int>, int>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int>, NonExistent>);

    // ==========================================
    // <int, X> order for the LHS
    // ==========================================
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, int>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, X>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, NonExistent>);
    // ------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X, int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<int,    NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X,      NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<int, X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X, int, NonExistent>>);

    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X, int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X, NonExistent>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X, NonExistent>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X, NonExistent>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X, NonExistent>, yk::rvariant<X, int>>);
    // ------------------------------------------

    // ==========================================
    // <X, int> order for the LHS
    // ==========================================
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int>, int>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int>, X>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, NonExistent>);
    // ------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<X, int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<NonExistent, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<NonExistent, int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<NonExistent, X, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<int,    NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<X,      NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<int, X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<X, int, NonExistent>>);

    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, int>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, int>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, int>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, int>, yk::rvariant<X, int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int, NonExistent>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int, NonExistent>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int, NonExistent>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int, NonExistent>, yk::rvariant<X, int>>);
    // ------------------------------------------
}

TEST_CASE("flexible", "[flexible][wrapper]")
{
    struct X {};
    struct NonExistent {};
    using RW_int = yk::recursive_wrapper<int>;
    // ------------------------------------------
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int>, RW_int>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int>, RW_int>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int>, int>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int>, NonExistent>);

    // ==========================================
    // <int, X> order for the LHS
    // ==========================================
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<X, int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<NonExistent, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<NonExistent, int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<NonExistent, X, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<int,    NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<X,      NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<int, X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<X, int, NonExistent>>);

    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, RW_int, X>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, RW_int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, RW_int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, RW_int, X>, yk::rvariant<X, int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X, NonExistent>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X, NonExistent>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X, NonExistent>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X, NonExistent>, yk::rvariant<X, int>>);
    // ------------------------------------------
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X, RW_int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<RW_int,    NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X,         NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<RW_int, X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X, RW_int, NonExistent>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X, NonExistent>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X, NonExistent>, yk::rvariant<X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X, NonExistent>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X, NonExistent>, yk::rvariant<X, RW_int>>);
    // ------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<X, RW_int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<NonExistent, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<NonExistent, RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<NonExistent, X, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<RW_int,    NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<X,         NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<RW_int, X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<RW_int, X>, yk::rvariant<X, RW_int, NonExistent>>);

    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, RW_int, X>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, RW_int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, RW_int, X>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, RW_int, X>, yk::rvariant<X, RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X, NonExistent>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X, NonExistent>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X, NonExistent>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<RW_int, X, NonExistent>, yk::rvariant<X, RW_int>>);

    // ==========================================
    // <X, int> order for the LHS
    // ==========================================
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<X, int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<NonExistent, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<NonExistent, int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<NonExistent, X, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<int, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<int, X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<X, int, NonExistent>>);

    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, RW_int>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, RW_int>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, RW_int>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, RW_int>, yk::rvariant<X, int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int, NonExistent>, yk::rvariant<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int, NonExistent>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int, NonExistent>, yk::rvariant<int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int, NonExistent>, yk::rvariant<X, int>>);
    // ------------------------------------------
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<X, RW_int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<NonExistent, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<NonExistent, RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<NonExistent, X, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<RW_int,    NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<X,         NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<RW_int, X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int>, yk::rvariant<X, RW_int, NonExistent>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, X, int>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, int>, yk::rvariant<X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, X, int>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, X, int>, yk::rvariant<X, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int, NonExistent>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, int, NonExistent>, yk::rvariant<X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int, NonExistent>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, int, NonExistent>, yk::rvariant<X, RW_int>>);
    // ------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<X, RW_int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<NonExistent, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<NonExistent, RW_int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<NonExistent, X, RW_int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<RW_int,    NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<X,         NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<RW_int, X, NonExistent>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<X, RW_int>, yk::rvariant<X, RW_int, NonExistent>>);

    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, RW_int>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, RW_int>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, RW_int>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, X, RW_int>, yk::rvariant<X, RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int, NonExistent>, yk::rvariant<RW_int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int, NonExistent>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int, NonExistent>, yk::rvariant<RW_int, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<X, RW_int, NonExistent>, yk::rvariant<X, RW_int>>);
}

TEST_CASE("ambiguous flexible", "[flexible]")
{
    {
        struct A {};
        struct B {};

        {
            using V = yk::rvariant<std::monostate, A, B, yk::rvariant<A, B>>;
            // should call generic constructor
            V v{yk::rvariant<A, B>{}};
            CHECK(yk::holds_alternative<yk::rvariant<A, B>>(v) == true);
        }
        {
            using V = yk::rvariant<std::monostate, A, B, yk::rvariant<A, B>, yk::rvariant<A, B>>;
            // ill-formed; flexible constructor is not eligible, and
            // generic constructor's imaginary function has ambiguous overload
            STATIC_CHECK(!std::is_constructible_v<V, yk::rvariant<A, B>>);
        }

        {
            using V = yk::rvariant<std::monostate, A, B, yk::rvariant<A, B>>;
            // should call generic assignment operator
            V v;
            v = yk::rvariant<A, B>{};
            CHECK(yk::holds_alternative<yk::rvariant<A, B>>(v) == true);
        }
        {
            using V = yk::rvariant<std::monostate, A, B, yk::rvariant<A, B>, yk::rvariant<A, B>>;
            // ill-formed; flexible assignment operator is not eligible, and
            // generic assignment operator's imaginary function has ambiguous overload
            STATIC_CHECK(!std::is_assignable_v<V&, yk::rvariant<A, B>>);
        }
    }
}

TEST_CASE("flexible copy construction", "[flexible]")
{
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);
        yk::rvariant<int, float> b = a;
        CHECK(b.index() == 0);
        yk::rvariant<int, float, double> c = b;
        CHECK(c.index() == 0);
    }
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);
        yk::rvariant<float, int> b = a;
        CHECK(b.index() == 1);
        yk::rvariant<double, float, int> c = b;
        CHECK(c.index() == 2);
    }
    {
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, float, MC_Thrower> a(valueless);
        CHECK(a.valueless_by_exception());
    }
}

TEST_CASE("flexible move construction", "[flexible]")
{
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);
        yk::rvariant<int, float> b = std::move(a);
        CHECK(b.index() == 0);
        yk::rvariant<int, float, double> c = std::move(b);
        CHECK(c.index() == 0);
    }
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);
        yk::rvariant<float, int> b = std::move(a);
        CHECK(b.index() == 1);
        yk::rvariant<double, float, int> c = std::move(b);
        CHECK(c.index() == 2);
    }
    {
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, float, MC_Thrower> a(std::move(valueless));
        CHECK(a.valueless_by_exception());
    }
}

TEST_CASE("flexible copy assignment", "[flexible]")
{
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);

        yk::rvariant<int, float> b = 3.14f;
        CHECK(b.index() == 1);
        CHECK_NOTHROW(b = a);
        CHECK(b.index() == 0);
    }
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);

        yk::rvariant<int, float, int> b(std::in_place_index<2>, 33 - 4);
        CHECK(b.index() == 2);
        CHECK_NOTHROW(b = a);
        CHECK(b.index() == 2);  // b's contained value is directly assigned from a's contained value, no alternative changed
    }
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);

        yk::rvariant<int, float, int> b(std::in_place_index<0>, 33 - 4);
        CHECK(b.index() == 0);
        CHECK_NOTHROW(b = a);
        CHECK(b.index() == 0);  // b's contained value is directly assigned from a's contained value, no alternative changed
    }
    {
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, float, MC_Thrower> a;
        a = valueless;
        CHECK(a.valueless_by_exception());
    }
}

TEST_CASE("flexible move assignment", "[flexible]")
{
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);

        yk::rvariant<int, float> b = 3.14f;
        CHECK(b.index() == 1);
        CHECK_NOTHROW(b = std::move(a));
        CHECK(b.index() == 0);
    }
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);

        yk::rvariant<int, float, int> b(std::in_place_index<2>, 33 - 4);
        CHECK(b.index() == 2);
        CHECK_NOTHROW(b = std::move(a));
        CHECK(b.index() == 2);  // b's contained value is directly assigned from a's contained value, no alternative changed
    }
    {
        yk::rvariant<int> a = 42;
        CHECK(a.index() == 0);

        yk::rvariant<int, float, int> b(std::in_place_index<0>, 33 - 4);
        CHECK(b.index() == 0);
        CHECK_NOTHROW(b = std::move(a));
        CHECK(b.index() == 0);  // b's contained value is directly assigned from a's contained value, no alternative changed
    }
    {
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, float, MC_Thrower> a;
        a = std::move(valueless);
        CHECK(a.valueless_by_exception());
    }
}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4244) // implicit numeric conversion
#endif

TEST_CASE("subset", "[flexible]")
{
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<int, float>>(0) == 0);
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<int, float>>(1) == 1);

    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<float, int>>(0) == 1);
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<float, int>>(1) == 0);

    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<int, float, double>>(0) == 0);
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<int, float, double>>(1) == 1);

    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<float, double, int>>(0) == 2);
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<float, double, int>>(1) == 0);

    {
        yk::rvariant<int> a{42};
        CHECK_NOTHROW(yk::rvariant<int>{a.subset<int>()});
        CHECK_NOTHROW(yk::rvariant<int>{std::as_const(a).subset<int>()});
        CHECK_NOTHROW(yk::rvariant<int>{std::move(std::as_const(a)).subset<int>()});
        CHECK_NOTHROW(yk::rvariant<int>{std::move(a).subset<int>()});
    }
    {
        yk::rvariant<int, float> a{42};
        CHECK_NOTHROW(yk::rvariant<int>{a.subset<int>()});
        CHECK_NOTHROW(yk::rvariant<int>{std::as_const(a).subset<int>()});
        CHECK_NOTHROW(yk::rvariant<int>{std::move(std::as_const(a)).subset<int>()});
        CHECK_NOTHROW(yk::rvariant<int>{std::move(a).subset<int>()});
    }
    {
        yk::rvariant<int, float> a{42};
        REQUIRE_THROWS(a.subset<float>());
    }
    {
        yk::rvariant<int, float, MC_Thrower> valueless = make_valueless<int, float>();
        yk::rvariant<int, MC_Thrower> a = valueless.subset<int, MC_Thrower>();
        CHECK(a.valueless_by_exception());
    }
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

} // unit_test
