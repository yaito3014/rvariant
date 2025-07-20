#include "yk/rvariant/recursive_wrapper.hpp"
#include "yk/rvariant/rvariant.hpp"

#include <catch2/catch_test_macros.hpp>

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4244) // implicit numeric conversion
#endif

TEST_CASE("truly recursive", "[recursive]")
{
    // Although this pattern is perfectly valid in type level,
    // it inherently allocates infinite amount of memory.
    // We just need to make sure it has the correct type traits.
    {
        struct SubExpr;
        using Expr = yk::rvariant<yk::recursive_wrapper<SubExpr>>;
        struct SubExpr { Expr expr; };

        STATIC_REQUIRE( std::is_constructible_v<Expr, Expr>);
        STATIC_REQUIRE( std::is_constructible_v<Expr, SubExpr>);
        STATIC_REQUIRE(!std::is_constructible_v<Expr, int>);

        STATIC_REQUIRE( std::is_constructible_v<SubExpr, SubExpr>);
        STATIC_REQUIRE( std::is_constructible_v<SubExpr, Expr>);
        STATIC_REQUIRE(!std::is_constructible_v<SubExpr, int>);

        //Expr expr; // infinite malloc
    }

    // In contrast to above, this pattern has a safe *fallback* of int
    {
        // Sanity check
        {
            using V = yk::rvariant<yk::recursive_wrapper<int>>;
            STATIC_REQUIRE( std::is_constructible_v<V, V>);
            STATIC_REQUIRE( std::is_constructible_v<V, int>);
            STATIC_REQUIRE( std::is_constructible_v<V, double>);  // !!true!!
        }

        struct SubExpr;
        using Expr = yk::rvariant<int, yk::recursive_wrapper<SubExpr>>;
        struct SubExpr { Expr expr; };

        STATIC_REQUIRE( std::is_constructible_v<Expr, int>);
        STATIC_REQUIRE(!std::is_constructible_v<Expr, double>); // false because equally viable

        //STATIC_REQUIRE(std::is_constructible_v<SubExpr, int>); // why fail in MSVC?????
        STATIC_REQUIRE( std::is_constructible_v<SubExpr, int&&>); // ok
        STATIC_REQUIRE(!std::is_constructible_v<SubExpr, double>); // false because equally viable

        REQUIRE_NOTHROW(Expr{});
        REQUIRE_NOTHROW(SubExpr{});
        REQUIRE_NOTHROW(Expr{42});
        REQUIRE_NOTHROW(SubExpr{42}); // OK in MSVC.... (in contrast to failure above)
    }
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif
