#include "yk/rvariant/recursive_wrapper.hpp"
#include "yk/rvariant/rvariant.hpp"
#include "yk/rvariant/pack.hpp"

#include "yk/indirect.hpp"

#include <catch2/catch_test_macros.hpp>

#include <type_traits>
#include <utility>
#include <variant>
#include <vector>


TEST_CASE("pack indexing")
{
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int, float>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<1, int, float>, float>);
}

TEST_CASE("exactly once")
{
    STATIC_REQUIRE(yk::detail::exactly_once_v<int, yk::detail::type_list<int, float>>);
    STATIC_REQUIRE_FALSE(yk::detail::exactly_once_v<int, yk::detail::type_list<int, int>>);
}

TEST_CASE("is in")
{
    STATIC_REQUIRE(yk::detail::is_in_v<int, int, float>);
    STATIC_REQUIRE_FALSE(yk::detail::is_in_v<int, float>);
}

TEST_CASE("subset like")
{
    STATIC_REQUIRE(yk::subset_of<yk::rvariant<int, float>, yk::rvariant<int, float, double>>);
    STATIC_REQUIRE(yk::subset_of<yk::rvariant<int, double>, yk::rvariant<int, float, double>>);
    STATIC_REQUIRE(yk::subset_of<yk::rvariant<float, double>, yk::rvariant<int, float, double>>);
    STATIC_REQUIRE(yk::subset_of<yk::rvariant<int, float>, yk::rvariant<float, int>>);

    STATIC_REQUIRE_FALSE(yk::subset_of<yk::rvariant<int>, yk::rvariant<double>>);
    STATIC_REQUIRE_FALSE(yk::subset_of<yk::rvariant<int, float, double>, yk::rvariant<int>>);
    STATIC_REQUIRE_FALSE(yk::subset_of<yk::rvariant<int, float, double>, yk::rvariant<int, float>>);
}

TEST_CASE("find index")
{
    STATIC_REQUIRE(yk::detail::find_index_v<int, yk::detail::type_list<int, float, double>> == 0);
    STATIC_REQUIRE(yk::detail::find_index_v<float, yk::detail::type_list<int, float, double>> == 1);
    STATIC_REQUIRE(yk::detail::find_index_v<double, yk::detail::type_list<int, float, double>> == 2);
    STATIC_REQUIRE(yk::detail::find_index_v<int, yk::detail::type_list<float, double>> == yk::detail::find_index_npos);
}

TEST_CASE("subset_reindex")
{
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<int, float>>(0) == 0);
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<int, float>>(1) == 1);

    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<float, int>>(0) == 1);
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<float, int>>(1) == 0);

    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<int, float, double>>(0) == 0);
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<int, float, double>>(1) == 1);

    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<float, double, int>>(0) == 2);
    STATIC_REQUIRE(yk::detail::subset_reindex<yk::rvariant<int, float>, yk::rvariant<float, double, int>>(1) == 0);
}

TEST_CASE("pack union")
{
    using yk::detail::pack_union_t;
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, int, float>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, yk::rvariant<int>, float>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, int, yk::rvariant<float>>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, yk::rvariant<int>, yk::rvariant<float>>, yk::rvariant<int, float>>);

    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, int, int>, yk::rvariant<int>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, yk::rvariant<int, float>, yk::rvariant<int, double>>, yk::rvariant<int, float, double>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, yk::rvariant<float, int>, yk::rvariant<int, double>>, yk::rvariant<float, int, double>>);
}

TEST_CASE("compact alternative")
{
    STATIC_REQUIRE(std::is_same_v<yk::compact_alternative_t<yk::rvariant, int, float>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<yk::compact_alternative_t<yk::rvariant, int, int>, int>);
}

TEST_CASE("helper class")
{
    STATIC_REQUIRE(yk::variant_size_v<yk::rvariant<int>> == 1);
    STATIC_REQUIRE(yk::variant_size_v<yk::rvariant<int, float>> == 2);

    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int>>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int, float>>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<1, yk::rvariant<int, float>>, float>);

    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int> const>, int const>);
    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int, float> const>, int const>);
    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<1, yk::rvariant<int, float> const>, float const>);
}

TEST_CASE("variadic union")
{
    // NOLINTBEGIN(modernize-use-equals-default)
    {
        using VD = yk::detail::variadic_union<true, int, float>;

        STATIC_REQUIRE(std::is_nothrow_default_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_move_constructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_move_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copy_assignable_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<VD>);

        STATIC_REQUIRE(std::is_trivially_move_assignable_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_move_assignable_v<VD>);

        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, yk::detail::valueless_t>);

        STATIC_REQUIRE(std::is_trivially_destructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_destructible_v<VD>);
    }
    {
        struct S
        {
            S() noexcept {}
            S(S const&) noexcept {}
            S(S&&) noexcept {}
            S& operator=(S const&) noexcept { return *this; }
            S& operator=(S&&) noexcept { return *this; }
            ~S() noexcept {}
        };
        using VD = yk::detail::variadic_union<false, S>;

        STATIC_REQUIRE(std::is_nothrow_default_constructible_v<VD>);

        STATIC_REQUIRE(!std::is_copy_constructible_v<VD>); // requires index access
        STATIC_REQUIRE(!std::is_move_constructible_v<VD>); // requires index access
        STATIC_REQUIRE(!std::is_copy_assignable_v<VD>);    // requires index access
        STATIC_REQUIRE(!std::is_move_assignable_v<VD>);    // requires index access

        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, yk::detail::valueless_t>);

        STATIC_REQUIRE(std::is_nothrow_destructible_v<VD>);
    }
    {
        struct S
        {
            S() = default;
            S(int, float) {}
        };
        using VD = yk::detail::variadic_union<true, S>;

        STATIC_REQUIRE(std::is_nothrow_default_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_move_constructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_move_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copy_assignable_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<VD>);

        STATIC_REQUIRE(std::is_trivially_move_assignable_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_move_assignable_v<VD>);

        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, yk::detail::valueless_t>);

        STATIC_REQUIRE(std::is_nothrow_destructible_v<VD>);

        STATIC_REQUIRE(std::is_constructible_v<VD, std::in_place_index_t<0>, S>);
        STATIC_REQUIRE(std::is_constructible_v<VD, std::in_place_index_t<0>, int, float>);
    }
    // NOLINTEND(modernize-use-equals-default)
}

TEST_CASE("default construction")
{
    {
        yk::rvariant<int> var;
        STATIC_REQUIRE(std::is_nothrow_default_constructible_v<yk::rvariant<int>>);
        CHECK_FALSE(var.valueless_by_exception());
        CHECK(var.index() == 0);
    }
    {
        struct S
        {
            S() noexcept(false) {}
        };
        STATIC_REQUIRE_FALSE(std::is_nothrow_default_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_nothrow_default_constructible_v<yk::rvariant<S>>);
    }
    {
        struct S
        {
            S() = delete;
        };
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<yk::rvariant<S>>);
    }
    {
        struct S
        {
            S() { throw std::exception(); }
        };
        REQUIRE_THROWS(yk::rvariant<S>());
    }
}

TEST_CASE("copy construction")
{
    {
        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<yk::rvariant<int>>);
        yk::rvariant<int, double> a;
        yk::rvariant<int, double> b(a);
        CHECK_FALSE(b.valueless_by_exception());
        CHECK(b.index() == 0);
    }

    // NOLINTBEGIN(modernize-use-equals-default)
    {
        struct S
        {
            S(S const&) {}
        };
        STATIC_REQUIRE(std::is_copy_constructible_v<S>);
        STATIC_REQUIRE(std::is_copy_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE(std::is_copy_constructible_v<yk::rvariant<int, S>>);
        STATIC_REQUIRE_FALSE(std::is_trivially_copy_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_trivially_copy_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE_FALSE(std::is_trivially_copy_constructible_v<yk::rvariant<int, S>>);
    }
    {
        struct S
        {
            S(S const&) = delete;
        };
        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<yk::rvariant<int, S>>);
    }
    {
        struct S
        {
            S() = default;
            S(S const&) { throw std::exception(); }
        };
        yk::rvariant<S> a;
        REQUIRE_THROWS(yk::rvariant<S>(a));
    }
    // NOLINTEND(modernize-use-equals-default)

    // TODO: valueless case
}

TEST_CASE("move construction")
{
    {
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<yk::rvariant<int>>);
        yk::rvariant<int, double> a;
        yk::rvariant<int, double> b(std::move(a));
        CHECK_FALSE(b.valueless_by_exception());
        CHECK(b.index() == 0);
    }
    {
        struct S
        {
            S(S&&) noexcept {}
        };
        STATIC_REQUIRE_FALSE(std::is_trivially_move_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_trivially_move_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE_FALSE(std::is_trivially_move_constructible_v<yk::rvariant<int, S>>);
    }
    {
        struct S
        {
            S(S&&) = delete;
        };
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<yk::rvariant<int, S>>);
    }
    {
        struct S
        {
            S() = default;
            S(S&&) noexcept(false) { throw std::exception(); }
        };
        yk::rvariant<S> a;
        REQUIRE_THROWS(yk::rvariant<S>(std::move(a)));
    }

    // TODO: valueless case
}

TEST_CASE("construction with index")
{
    {
        yk::rvariant<int, float> var(std::in_place_index<0>, 42);
        REQUIRE_FALSE(var.valueless_by_exception());
        REQUIRE(var.index() == 0);
    }
    {
        yk::rvariant<int, float> var(std::in_place_index<1>, 3.14f);
        REQUIRE_FALSE(var.valueless_by_exception());
        REQUIRE(var.index() == 1);
    }
    {
        yk::rvariant<std::vector<int>> var(std::in_place_index<0>, {3, 1, 4});
    }
}

TEST_CASE("construction with type")
{
    {
        yk::rvariant<int, float> var(std::in_place_type<int>, 42);
        REQUIRE_FALSE(var.valueless_by_exception());
        REQUIRE(var.index() == 0);
    }
    {
        yk::rvariant<int, float> var(std::in_place_type<float>, 3.14f);
        REQUIRE_FALSE(var.valueless_by_exception());
        REQUIRE(var.index() == 1);
    }
    {
        yk::rvariant<std::vector<int>> var(std::in_place_type<std::vector<int>>, {3, 1, 4});
    }
}

TEST_CASE("recursive_sentinel")
{
    using yk::detail::recursive_sentinel_t;
    using yk::detail::recursive_sentinel;
    using yk::detail::type_list;

    {
        //std::variant<int, double> var(42);
    }

    // TODO
    {
        //using Sentinel = recursive_sentinel_t<type_list<int>>;
        //using V = yk::rvariant<int, double>;

        {
            //int value(Sentinel{});
        }
        {
            //double value(Sentinel{});
        }

        {
            //V var(Sentinel{});
        }

        {
            //yk::detail::FUN<Sentinel, V>{}();
        }

        {
            //V var(42);
        }
    }
}

TEST_CASE("generic construction")
{
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE_FALSE(var.valueless_by_exception());
        REQUIRE(var.index() == 0);
    }
    {
        yk::rvariant<int, float> var = 3.14f;
        REQUIRE_FALSE(var.valueless_by_exception());
        REQUIRE(var.index() == 1);
    }
}

TEST_CASE("flexible copy construction")
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

TEST_CASE("flexible move construction")
{
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<int, float> b = std::move(a);
        REQUIRE(b.index() == 0);
        yk::rvariant<int, float, double> c = std::move(b);
        REQUIRE(c.index() == 0);
    }
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<float, int> b = std::move(a);
        REQUIRE(b.index() == 1);
        yk::rvariant<double, float, int> c = std::move(b);
        REQUIRE(c.index() == 2);
    }

    // TODO: valueless case
}

TEST_CASE("subset")
{
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

TEST_CASE("copy assign")
{
    // NOLINTBEGIN(modernize-use-equals-default)

    // trivial case
    {
        yk::rvariant<int, float> a = 42, b = 3.14f;
        REQUIRE_NOTHROW(a = b);  // different alternative
        REQUIRE(a.index() == 1);
        REQUIRE_NOTHROW(a = b);  // same alternative
        REQUIRE(a.index() == 1);
    }

    // non-trivial case
    {
        struct S
        {
            S() {}
            S(S const&) noexcept {}
            S(S&&) noexcept(false) { throw std::exception{}; }
            S& operator=(S const&) noexcept { return *this; }
            S& operator=(S&&) noexcept(false) { throw std::exception{}; }
        };
        yk::rvariant<S, int> a = 42, b;
        REQUIRE_NOTHROW(a = b);  // different alternative; use copy constructor
        REQUIRE_NOTHROW(a = b);  // same alternative; directly use copy assignment
    }
    {
        struct S
        {
            S() {}
            S(S const&) noexcept(false) { throw std::exception{}; }
            S(S&&) noexcept {}
            S& operator=(S const&) noexcept { return *this; }
            S& operator=(S&&) noexcept { return *this; }
        };
        yk::rvariant<S, int> a = 42, b;
        REQUIRE_THROWS(a = b);  // different alternative; move temporary copy
    }

    // NOLINTEND(modernize-use-equals-default)
}

TEST_CASE("move assign")
{
    // trivial case
    {
        yk::rvariant<int, float> a = 42, b = 3.14f;
        REQUIRE_NOTHROW(a = std::move(b));  // different alternative
        REQUIRE(a.index() == 1);
    }
    {
        yk::rvariant<int, float> a = 42, b = 33 - 4;
        REQUIRE_NOTHROW(a = std::move(b));  // same alternative
        REQUIRE(a.index() == 0);
    }

    // non-trivial case
    {
        struct S
        {
            S() = default;
            S(S const&) noexcept(false) { throw std::exception{}; }
            S(S&&) noexcept {}
            S& operator=(S const&) noexcept(false) { throw std::exception{}; }
            S& operator=(S&&) noexcept { return *this; }
        };
        {
            yk::rvariant<S, int> a = 42, b;
            CHECK_NOTHROW(a = std::move(b));  // different alternative; use move constructor
            CHECK(a.index() == 0);
        }
        {
            yk::rvariant<S, int> a, b;
            CHECK_NOTHROW(a = std::move(b));  // same alternative; directly use move assignment
            CHECK(a.index() == 0);
        }
    }
}

TEST_CASE("flexible copy assign")
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

TEST_CASE("flexible move assign")
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

TEST_CASE("generic assign")
{
    {
        yk::rvariant<int, float> a{42};
        CHECK(a.index() == 0);

        a = 33;
        CHECK(a.index() == 0);

        a = 3.14f;
        CHECK(a.index() == 1);
    }
}

TEST_CASE("emplace")
{
    {
        yk::rvariant<int> a = 42;
        REQUIRE_NOTHROW(a.emplace<int>(12));
        REQUIRE_NOTHROW(a.emplace<0>(12));
    }
    {
        yk::rvariant<std::vector<int>> a;
        a.emplace<std::vector<int>>({3, 1, 4});
        a.emplace<0>({3, 1, 4});
    }
    {
        yk::rvariant<int, float> a = 42;
        a.emplace<1>(3.14f);
        REQUIRE(a.index() == 1);
    }
}

TEST_CASE("raw get")
{
    yk::rvariant<int, float> var = 42;
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::raw_get<0>(var)), yk::detail::alternative<0, int>&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::raw_get<0>(std::move(var))), yk::detail::alternative<0, int>&&>);
}

TEST_CASE("get")
{
    // Required for suppressing std::move(const&)
    // NOLINTBEGIN(performance-move-const-arg)
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(yk::get<0>(std::as_const(var)) == 42);
        REQUIRE(yk::get<0>(var) == 42);
        REQUIRE(yk::get<0>(std::move(std::as_const(var))) == 42);
        REQUIRE(yk::get<0>(std::move(var)) == 42);
        REQUIRE_THROWS(yk::get<1>(var));
    }
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(yk::get<int>(std::as_const(var)) == 42);
        REQUIRE(yk::get<int>(var) == 42);
        REQUIRE(yk::get<int>(std::move(std::as_const(var))) == 42);
        REQUIRE(yk::get<int>(std::move(var)) == 42);
        REQUIRE_THROWS(yk::get<float>(var));
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(yk::get<0>(std::as_const(var)) == 42);
        REQUIRE(yk::get<0>(var) == 42);
        REQUIRE(yk::get<0>(std::move(std::as_const(var))) == 42);
        REQUIRE(yk::get<0>(std::move(var)) == 42);
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(yk::get<int>(std::as_const(var)) == 42);
        REQUIRE(yk::get<int>(var) == 42);
        REQUIRE(yk::get<int>(std::move(std::as_const(var))) == 42);
        REQUIRE(yk::get<int>(std::move(var)) == 42);
    }
    // NOLINTEND(performance-move-const-arg)
}

TEST_CASE("get_if")
{
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(*yk::get_if<0>(&var) == 42);
        REQUIRE(*yk::get_if<0>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(*yk::get_if<0>(&var) == 42);
        REQUIRE(*yk::get_if<0>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(*yk::get<0>(&var) == 42);
        REQUIRE(*yk::get<0>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(*yk::get<0>(&var) == 42);
        REQUIRE(*yk::get<0>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(*yk::get_if<int>(&var) == 42);
        REQUIRE(*yk::get_if<int>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(*yk::get_if<int>(&var) == 42);
        REQUIRE(*yk::get_if<int>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(*yk::get<int>(&var) == 42);
        REQUIRE(*yk::get<int>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(*yk::get<int>(&var) == 42);
        REQUIRE(*yk::get<int>(&std::as_const(var)) == 42);
    }
}

TEST_CASE("swap")
{
    {
        yk::rvariant<int> a = 33, b = 4;
        REQUIRE_NOTHROW(a.swap(b));
        REQUIRE(yk::get<0>(a) == 4);
        REQUIRE(yk::get<0>(b) == 33);
    }
    {
        yk::rvariant<int, float> a = 42, b = 3.14f;
        REQUIRE_NOTHROW(a.swap(b));
        REQUIRE(yk::get<1>(a) == 3.14f);
        REQUIRE(yk::get<0>(b) == 42);
    }

    // TODO: valueless case
}

TEST_CASE("holds_alternative")
{
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(yk::holds_alternative<int>(var));
    }
}

TEST_CASE("recursive wrapper")
{
    {
        yk::recursive_wrapper<int> a(42);
        CHECK_FALSE(a.valueless_after_move());
    }

    // NOLINTBEGIN(performance-unnecessary-value-param)

    CHECK([](std::variant<yk::recursive_wrapper<int>> var) {
        return std::holds_alternative<yk::recursive_wrapper<int>>(var);
    }(42));
    CHECK([](yk::rvariant<yk::recursive_wrapper<int>> var) {
        return yk::holds_alternative<int>(var);
    }(42));

    CHECK([](std::variant<yk::recursive_wrapper<int>> var) {
        return std::holds_alternative<yk::recursive_wrapper<int>>(var);
    }(3.14));
    CHECK([](yk::rvariant<yk::recursive_wrapper<int>> var) {
        return yk::holds_alternative<int>(var);
    }(3.14));

    CHECK([](std::variant<yk::recursive_wrapper<int>, double> var) {
        return std::holds_alternative<double>(var);
    }(3.14f));
    CHECK([](yk::rvariant<yk::recursive_wrapper<int>, double> var) {
        return yk::holds_alternative<double>(var);
    }(3.14f));

    CHECK([](std::variant<yk::recursive_wrapper<double>, int> var) {
        return std::holds_alternative<yk::recursive_wrapper<double>>(var);
    }(3.14));
    CHECK([](yk::rvariant<yk::recursive_wrapper<double>, int> var) {
        return yk::holds_alternative<double>(var);
    }(3.14));

    CHECK([](std::variant<yk::recursive_wrapper<double>, int> var) {
        return std::holds_alternative<yk::recursive_wrapper<double>>(var);
    }(3.14f));
    CHECK([](yk::rvariant<yk::recursive_wrapper<double>, int> var) {
        return yk::holds_alternative<double>(var);
    }(3.14f));

    CHECK([](std::variant<yk::recursive_wrapper<double>, int> var) {
        return std::holds_alternative<int>(var);
    }(3));
    CHECK([](yk::rvariant<yk::recursive_wrapper<double>, int> var) {
        return yk::holds_alternative<int>(var);
    }(3));

    CHECK([](std::variant<yk::recursive_wrapper<double>, int> var) {
        return std::holds_alternative<yk::recursive_wrapper<double>>(var);
    }(yk::recursive_wrapper<double>(3.14)));
    CHECK([](yk::rvariant<yk::recursive_wrapper<double>, int> var) {
        return yk::holds_alternative<double>(var);
    }(yk::recursive_wrapper<double>(3.14)));

    // NOLINTEND(performance-unnecessary-value-param)

    {
        yk::rvariant<yk::recursive_wrapper<int>> var = 42;
        var = 33 - 4;
        CHECK(yk::holds_alternative<int>(var));
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>> var = 42;
        var = yk::recursive_wrapper<int>{};
        CHECK(yk::holds_alternative<int>(var));
    }

    {
        struct S
        {
            S(int i, double d) {}
        };

        yk::rvariant<S> var(std::in_place_index<0>, 42, 3.14);
        (void)var;
    }

    {
        yk::recursive_wrapper<int> wrapper(std::in_place, 42);
        CHECK(*wrapper == 42);
        yk::rvariant<yk::recursive_wrapper<int>> a(std::in_place_index<0>);
        yk::rvariant<yk::recursive_wrapper<int>> b(std::in_place_index<0>, 42);
        yk::rvariant<yk::recursive_wrapper<int>> c(std::in_place_index<0>, std::in_place);
        yk::rvariant<yk::recursive_wrapper<int>> d(std::in_place_index<0>, std::in_place, 42);
        yk::rvariant<yk::recursive_wrapper<int>> e(std::in_place_index<0>, std::allocator_arg, std::allocator<int>{});
    }
}

TEST_CASE("maybe_wrapped")
{
    STATIC_REQUIRE(std::same_as<yk::detail::select_maybe_wrapped_t<int, int>, int>);
    STATIC_REQUIRE(yk::detail::select_maybe_wrapped_index<int, int> == 0);

    STATIC_REQUIRE(std::same_as<yk::detail::select_maybe_wrapped_t<int, double, int>, int>);
    STATIC_REQUIRE(yk::detail::select_maybe_wrapped_index<int, double, int> == 1);

    STATIC_REQUIRE(std::same_as<yk::detail::select_maybe_wrapped_t<int, yk::recursive_wrapper<int>>, yk::recursive_wrapper<int>>);
    STATIC_REQUIRE(yk::detail::select_maybe_wrapped_index<int, yk::recursive_wrapper<int>> == 0);

    STATIC_REQUIRE(std::same_as<yk::detail::select_maybe_wrapped_t<int, double, yk::recursive_wrapper<int>>, yk::recursive_wrapper<int>>);
    STATIC_REQUIRE(yk::detail::select_maybe_wrapped_index<int, double, yk::recursive_wrapper<int>> == 1);

    {
        yk::rvariant<double, int> v{std::in_place_type<int>, 42};
        CHECK(v.index() == 1);

        v.emplace<double>(3.14);
        CHECK(v.index() == 0);

        v.emplace<int>(43);
        CHECK(v.index() == 1);
    }
    {
        yk::rvariant<double, yk::recursive_wrapper<int>> v{std::in_place_type<int>, 42};
        CHECK(v.index() == 1);

        v.emplace<double>(3.14);
        CHECK(v.index() == 0);

        v.emplace<int>(43);
        CHECK(v.index() == 1);
    }
}

template<class... Ts>
constexpr auto FUN_not_UB = []<class T>(T&& t) constexpr {
    yk::detail::FUN<T, yk::rvariant<Ts...>>{}(std::forward<T>(t));
    return true;
};

TEST_CASE("non_recursive_same_as_std")
{
    {
        STATIC_REQUIRE(FUN_not_UB<int>(42));
        STATIC_REQUIRE(FUN_not_UB<int, double>(42));
    }

    {
        using V = std::variant<int>;
        STATIC_REQUIRE(std::is_constructible_v<V, V>);
        STATIC_REQUIRE(std::is_constructible_v<V, int>);
        STATIC_REQUIRE(!std::is_constructible_v<V, double>); // !!false!!
        //V{3.14}; // ill-formed
    }
    {
        using V = yk::rvariant<int>;
        STATIC_REQUIRE(std::is_constructible_v<V, V>);
        STATIC_REQUIRE(std::is_constructible_v<V, int>);
        STATIC_REQUIRE(!std::is_constructible_v<V, double>); // !!false!!
        //V{3.14}; // ill-formed
    }
}

TEST_CASE("truly recursive")
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
#if 0 // TODO
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
#endif
}

TEST_CASE("unwrap_recursive")
{
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<int&>())), int&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<int&&>())), int&&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<int const&>())), int const&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<int const&&>())), int const&&>);

    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<yk::recursive_wrapper<int>&>())), int&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<yk::recursive_wrapper<int>&&>())), int&&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<yk::recursive_wrapper<int> const&>())), int const&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<yk::recursive_wrapper<int> const&&>())), int const&&>);
}
