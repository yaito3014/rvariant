#include "yk/rvariant.hpp"

#include <catch2/catch_test_macros.hpp>

template<class A, class B, class Expected>
constexpr bool is_same_alt = std::is_same_v<yk::compact_alternative_t<yk::rvariant, A, B>, Expected>;

TEST_CASE("alternative", "[flexible]")
{
    STATIC_REQUIRE(is_same_alt<yk::rvariant<int>, yk::rvariant<int>, int>);
    STATIC_REQUIRE(is_same_alt<yk::rvariant<int>, yk::rvariant<double>, yk::rvariant<int, double>>);

    STATIC_REQUIRE(is_same_alt<yk::rvariant<int, double>, yk::rvariant<char, wchar_t>, yk::rvariant<int, double, char, wchar_t>>);
}

TEST_CASE("flexible", "[flexible]")
{
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int>, int>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int>, yk::recursive_wrapper<int>>);

    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>>, yk::recursive_wrapper<int>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>>, int>);

    struct X {};
    struct NonExistent {};

    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<int, X>, int>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<int, X>, X>);

    // ------------------------------------------
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<int>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X, int>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X, int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X, int>>);

    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<int>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X, int>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X, int>>);
    // ------------------------------------------
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<int>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<X, int>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<X, int>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, X, int>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, int, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, X, int>>);

    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<int>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<X, int>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<int, X>>);
    STATIC_REQUIRE(std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<X, int>>);
    // ------------------------------------------
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X, yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<X, yk::recursive_wrapper<int>>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X, yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<int, X>, yk::rvariant<NonExistent, X, yk::recursive_wrapper<int>>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X, yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<NonExistent, int, X>, yk::rvariant<X, yk::recursive_wrapper<int>>>);
    // ------------------------------------------
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<X, yk::recursive_wrapper<int>>>);

    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, X, yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE(!std::is_constructible_v<yk::rvariant<yk::recursive_wrapper<int>, X>, yk::rvariant<NonExistent, X, yk::recursive_wrapper<int>>>);

    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<yk::recursive_wrapper<int>>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<yk::recursive_wrapper<int>, X>>);
    STATIC_REQUIRE( std::is_constructible_v<yk::rvariant<NonExistent, yk::recursive_wrapper<int>, X>, yk::rvariant<X, yk::recursive_wrapper<int>>>);
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

    // TODO: valueless case
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

    // TODO: valueless case
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
}

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

    // TODO: valueless case
}
