// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "yk/rvariant/recursive_wrapper.hpp"
#include "yk/rvariant/rvariant.hpp"

#include <catch2/catch_test_macros.hpp>

namespace unit_test {

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4244) // implicit numeric conversion
#endif

TEST_CASE("truly recursive", "[wrapper][recursive]")
{
    // Although this pattern is perfectly valid in type level,
    // it inherently allocates infinite amount of memory.
    // We just need to make sure it has the correct type traits.

    // std::variant
    {
        struct SubExpr;
        using Expr = std::variant<yk::recursive_wrapper<SubExpr>>;
        struct SubExpr { Expr expr; };

        STATIC_REQUIRE( std::is_constructible_v<Expr, Expr>);
        STATIC_REQUIRE( std::is_constructible_v<Expr, SubExpr>);
        STATIC_REQUIRE(!std::is_constructible_v<Expr, int>); // this should result in infinite recursion if our implementation is wrong

        STATIC_REQUIRE( std::is_constructible_v<SubExpr, SubExpr>);
        STATIC_REQUIRE( std::is_constructible_v<SubExpr, Expr>);
        STATIC_REQUIRE(!std::is_constructible_v<SubExpr, int>);

        //Expr expr; // infinite malloc
    }

    // rvariant
    {
        struct SubExpr;
        using Expr = yk::rvariant<yk::recursive_wrapper<SubExpr>>;
        struct SubExpr { Expr expr; };

        STATIC_REQUIRE( std::is_constructible_v<Expr, Expr>);
        STATIC_REQUIRE( std::is_constructible_v<Expr, SubExpr>);
        STATIC_REQUIRE(!std::is_constructible_v<Expr, int>); // this should result in infinite recursion if our implementation is wrong

        STATIC_REQUIRE( std::is_constructible_v<SubExpr, SubExpr>);
        STATIC_REQUIRE( std::is_constructible_v<SubExpr, Expr>);
        STATIC_REQUIRE(!std::is_constructible_v<SubExpr, int>);

        //Expr expr; // infinite malloc
    }

    // In contrast to above, this pattern has a safe *fallback* of int
    {
        // Sanity check
        {
            using V = std::variant<int>;
            STATIC_REQUIRE( std::is_constructible_v<V, V>);
            STATIC_REQUIRE( std::is_constructible_v<V, int>);
            STATIC_REQUIRE(!std::is_constructible_v<V, double>);
        }
        {
            using V = std::variant<yk::recursive_wrapper<int>>;
            STATIC_REQUIRE( std::is_constructible_v<V, V>);
            STATIC_REQUIRE( std::is_constructible_v<V, int>);
            STATIC_REQUIRE( std::is_constructible_v<V, double>);  // !!true!!
        }
        {
            using V = yk::rvariant<yk::recursive_wrapper<int>>;
            STATIC_REQUIRE( std::is_constructible_v<V, V>);
            STATIC_REQUIRE( std::is_constructible_v<V, int>);
            STATIC_REQUIRE( std::is_constructible_v<V, double>);  // !!true!!
        }

        // Sanity check
        {
            struct SubExpr;
            using Expr = std::variant<int, yk::recursive_wrapper<SubExpr>>;
            struct SubExpr { Expr expr; };

            STATIC_REQUIRE( std::is_constructible_v<Expr, int>);
            STATIC_REQUIRE(!std::is_constructible_v<Expr, double>); // false because equally viable

            STATIC_REQUIRE( std::is_constructible_v<SubExpr, int>);
            STATIC_REQUIRE(!std::is_constructible_v<SubExpr, double>); // false because equally viable
        }

        struct SubExpr;
        using Expr = yk::rvariant<int, yk::recursive_wrapper<SubExpr>>;
        struct SubExpr { Expr expr; };

        STATIC_REQUIRE( std::is_constructible_v<Expr, int>);
        STATIC_REQUIRE(!std::is_constructible_v<Expr, double>); // false because equally viable

        STATIC_REQUIRE( std::is_constructible_v<SubExpr, int>);
        STATIC_REQUIRE(!std::is_constructible_v<SubExpr, double>); // false because equally viable

        REQUIRE_NOTHROW(Expr{});
        REQUIRE_NOTHROW(SubExpr{});
        REQUIRE_NOTHROW(Expr{42});
        REQUIRE_NOTHROW(SubExpr{42});

        STATIC_REQUIRE(std::is_constructible_v<Expr, std::in_place_index_t<0>, int>);
        CHECK_NOTHROW(Expr{std::in_place_index<0>, 42});

        STATIC_REQUIRE(std::is_constructible_v<Expr, std::in_place_index_t<1>, SubExpr>);
        CHECK_NOTHROW(Expr{std::in_place_index<1>, SubExpr{42}});

        STATIC_REQUIRE(std::is_constructible_v<Expr, int>);
        CHECK_NOTHROW(Expr{42});

        constexpr auto vis = yk::overloaded{
            [](int const&)     { return 0; },
            [](SubExpr const&) { return 1; },
        };
        STATIC_CHECK(Expr{42}.visit(vis) == 0);
        STATIC_CHECK(Expr{SubExpr{Expr{42}}}.visit(vis) == 1);
    }
    {
        struct BinaryExpr;
        using Expr = yk::rvariant<int, double, yk::recursive_wrapper<BinaryExpr>>;
        enum class Op;
        struct BinaryExpr { Expr lhs, rhs; Op op{}; };

        Expr expr{BinaryExpr{Expr{42}, Expr{3.14}}};
        expr.visit(yk::overloaded{
            [](int const&) { /* ... */ },
            [](double const&) { /* ... */ },
            [](BinaryExpr const&) { /* ... */ },
        });
    }
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

} // unit_test
