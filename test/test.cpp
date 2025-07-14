#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <yk/detail/convert_index.hpp>
#include <yk/detail/exactly_once.hpp>
#include <yk/detail/find_index.hpp>
#include <yk/detail/is_in.hpp>
#include <yk/detail/pack_indexing.hpp>
#include <yk/detail/type_list.hpp>

#include <yk/recursive_wrapper.hpp>
#include <yk/rvariant.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("pack indexing") {
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int, float>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<1, int, float>, float>);
}

TEST_CASE("exactly once") {
    STATIC_REQUIRE(yk::detail::exactly_once_v<int, yk::detail::type_list<int, float>>);
    STATIC_REQUIRE_FALSE(yk::detail::exactly_once_v<int, yk::detail::type_list<int, int>>);
}

TEST_CASE("is in") {
    STATIC_REQUIRE(yk::detail::is_in_v<int, int, float>);
    STATIC_REQUIRE_FALSE(yk::detail::is_in_v<int, float>);
}

TEST_CASE("subset like") {
    STATIC_REQUIRE(yk::subset_of<yk::rvariant<int, float>, yk::rvariant<int, float, double>>);
    STATIC_REQUIRE(yk::subset_of<yk::rvariant<int, double>, yk::rvariant<int, float, double>>);
    STATIC_REQUIRE(yk::subset_of<yk::rvariant<float, double>, yk::rvariant<int, float, double>>);
    STATIC_REQUIRE(yk::subset_of<yk::rvariant<int, float>, yk::rvariant<float, int>>);

    STATIC_REQUIRE_FALSE(yk::subset_of<yk::rvariant<int>, yk::rvariant<double>>);
    STATIC_REQUIRE_FALSE(yk::subset_of<yk::rvariant<int, float, double>, yk::rvariant<int>>);
    STATIC_REQUIRE_FALSE(yk::subset_of<yk::rvariant<int, float, double>, yk::rvariant<int, float>>);
}

TEST_CASE("find index") {
    STATIC_REQUIRE(yk::detail::find_index_v<int, yk::detail::type_list<int, float, double>> == 0);
    STATIC_REQUIRE(yk::detail::find_index_v<float, yk::detail::type_list<int, float, double>> == 1);
    STATIC_REQUIRE(yk::detail::find_index_v<double, yk::detail::type_list<int, float, double>> == 2);
    STATIC_REQUIRE(yk::detail::find_index_v<int, yk::detail::type_list<float, double>> == yk::detail::find_index_npos);
}

TEST_CASE("convert index") {
    STATIC_REQUIRE(yk::detail::convert_index<yk::rvariant<int, float>, yk::rvariant<int, float>>(0) == 0);
    STATIC_REQUIRE(yk::detail::convert_index<yk::rvariant<int, float>, yk::rvariant<int, float>>(1) == 1);

    STATIC_REQUIRE(yk::detail::convert_index<yk::rvariant<int, float>, yk::rvariant<float, int>>(0) == 1);
    STATIC_REQUIRE(yk::detail::convert_index<yk::rvariant<int, float>, yk::rvariant<float, int>>(1) == 0);

    STATIC_REQUIRE(yk::detail::convert_index<yk::rvariant<int, float>, yk::rvariant<int, float, double>>(0) == 0);
    STATIC_REQUIRE(yk::detail::convert_index<yk::rvariant<int, float>, yk::rvariant<int, float, double>>(1) == 1);

    STATIC_REQUIRE(yk::detail::convert_index<yk::rvariant<int, float>, yk::rvariant<float, double, int>>(0) == 2);
    STATIC_REQUIRE(yk::detail::convert_index<yk::rvariant<int, float>, yk::rvariant<float, double, int>>(1) == 0);
}

TEST_CASE("pack union") {
    using yk::detail::pack_union_t;
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, int, float>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, yk::rvariant<int>, float>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, int, yk::rvariant<float>>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, yk::rvariant<int>, yk::rvariant<float>>, yk::rvariant<int, float>>);

    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, int, int>, yk::rvariant<int>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, yk::rvariant<int, float>, yk::rvariant<int, double>>, yk::rvariant<int, float, double>>);
    STATIC_REQUIRE(std::is_same_v<pack_union_t<yk::rvariant, yk::rvariant<float, int>, yk::rvariant<int, double>>, yk::rvariant<float, int, double>>);
}

TEST_CASE("compact alternative") {
    STATIC_REQUIRE(std::is_same_v<yk::compact_alternative_t<yk::rvariant, int, float>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<yk::compact_alternative_t<yk::rvariant, int, int>, int>);
}

TEST_CASE("helper class") {
    STATIC_REQUIRE(yk::variant_size_v<yk::rvariant<int>> == 1);
    STATIC_REQUIRE(yk::variant_size_v<yk::rvariant<int, float>> == 2);

    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int>>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int, float>>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<1, yk::rvariant<int, float>>, float>);

    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int> const>, int const>);
    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int, float> const>, int const>);
    STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<1, yk::rvariant<int, float> const>, float const>);
}

TEST_CASE("variadic union") {
    {
        using VD = yk::detail::variadic_union<true, int, float>;
        STATIC_REQUIRE(std::is_default_constructible_v<VD>);
        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<VD>);
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<VD>);
        STATIC_REQUIRE(std::is_trivially_destructible_v<VD>);
        STATIC_REQUIRE(std::is_constructible_v<VD, yk::detail::valueless_t>);
    }
    {
        using VD = yk::detail::variadic_union<false, std::vector<int>>;
        STATIC_REQUIRE(std::is_default_constructible_v<VD>);
        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<VD>);  // cannot copy without index
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<VD>);  // cannot move without index
        STATIC_REQUIRE(std::is_destructible_v<VD>);
    }
}

TEST_CASE("default construction") {
    {
        yk::rvariant<int> var;
        STATIC_REQUIRE(std::is_nothrow_default_constructible_v<yk::rvariant<int>>);
        CHECK_FALSE(var.valueless_by_exception());
        CHECK(var.index() == 0);
    }
    {
        struct S {
            S() noexcept(false) {}
        };
        STATIC_REQUIRE_FALSE(std::is_nothrow_default_constructible_v<S>);
        yk::rvariant<S> var;
        STATIC_REQUIRE_FALSE(std::is_nothrow_default_constructible_v<yk::rvariant<S>>);
    }
    {
        struct S {
            S() = delete;
        };
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_default_constructible_v<yk::rvariant<S>>);
    }
    {
        struct S {
            S() { throw std::exception(); }
        };
        REQUIRE_THROWS(yk::rvariant<S>());
    }
}

TEST_CASE("copy construction") {
    {
        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<yk::rvariant<int>>);
        yk::rvariant<int, double> a;
        yk::rvariant<int, double> b(a);
        CHECK_FALSE(b.valueless_by_exception());
        CHECK(b.index() == 0);
    }
    {
        struct S {
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
        struct S {
            S(S const&) = delete;
        };
        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<yk::rvariant<int, S>>);
    }
    {
        struct S {
            S() {}
            S(S const&) { throw std::exception(); }
        };
        yk::rvariant<S> a;
        REQUIRE_THROWS(yk::rvariant<S>(a));
    }
    // TODO: valueless case
}

TEST_CASE("move construction") {
    {
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<yk::rvariant<int>>);
        yk::rvariant<int, double> a;
        yk::rvariant<int, double> b(std::move(a));
        CHECK_FALSE(b.valueless_by_exception());
        CHECK(b.index() == 0);
    }
    {
        struct S {
            S(S&&) {}
        };
        STATIC_REQUIRE_FALSE(std::is_trivially_move_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_trivially_move_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE_FALSE(std::is_trivially_move_constructible_v<yk::rvariant<int, S>>);
    }
    {
        struct S {
            S(S&&) = delete;
        };
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<S>);
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE_FALSE(std::is_move_constructible_v<yk::rvariant<int, S>>);
    }
    {
        struct S {
            S() {}
            S(S&&) { throw std::exception(); }
        };
        yk::rvariant<S> a;
        REQUIRE_THROWS(yk::rvariant<S>(std::move(a)));
    }
    // TODO: valueless case
}

TEST_CASE("construction with index") {
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

TEST_CASE("construction with type") {
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

TEST_CASE("generic construction") {
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

TEST_CASE("flexible copy construction") {
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<int, float> b = a;
        REQUIRE(b.index() == 0);
        yk::rvariant<int, float, double> c = b;
        REQUIRE(c.index() == 0);
    }
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<float, int> b = a;
        REQUIRE(b.index() == 1);
        yk::rvariant<double, float, int> c = b;
        REQUIRE(c.index() == 2);
    }
    // TODO: valueless case
}

TEST_CASE("flexible move construction") {
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

TEST_CASE("subset") {
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<int> b = a.subset<int>();
        yk::rvariant<int> c = std::as_const(a).subset<int>();
        yk::rvariant<int> d = std::move(std::as_const(a)).subset<int>();
        yk::rvariant<int> e = std::move(a).subset<int>();
    }
    {
        yk::rvariant<int, float> a = 42;
        yk::rvariant<int> b = a.subset<int>();
        yk::rvariant<int> c = std::as_const(a).subset<int>();
        yk::rvariant<int> d = std::move(std::as_const(a)).subset<int>();
        yk::rvariant<int> e = std::move(a).subset<int>();
    }
    {
        yk::rvariant<int, float> a = 42;
        REQUIRE_THROWS(a.subset<float>());
    }
    // TODO: valueless case
}

TEST_CASE("copy assign") {
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
        struct S {
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
        struct S {
            S() {}
            S(S const&) noexcept(false) { throw std::exception{}; }
            S(S&&) noexcept {}
            S& operator=(S const&) noexcept { return *this; }
            S& operator=(S&&) noexcept { return *this; }
        };
        yk::rvariant<S, int> a = 42, b;
        REQUIRE_THROWS(a = b);  // different alternative; move temporary copy
    }
}

TEST_CASE("move assign") {
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
        struct S {
            S() {}
            S(S const&) noexcept(false) { throw std::exception{}; }
            S(S&&) noexcept {}
            S& operator=(S const&) noexcept(false) { throw std::exception{}; }
            S& operator=(S&&) noexcept { return *this; }
        };
        {
            yk::rvariant<S, int> a = 42, b;
            REQUIRE_NOTHROW(a = std::move(b));  // different alternative; use move constructor
            REQUIRE(a.index() == 0);
        }
        {
            yk::rvariant<S, int> a, b;
            REQUIRE_NOTHROW(a = std::move(b));  // same alternative; directly use move assignment
            REQUIRE(a.index() == 0);
        }
    }
}

TEST_CASE("flexible copy assign") {
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<int, float> b = 3.14f;
        REQUIRE_NOTHROW(b = a);
        REQUIRE(b.index() == 0);
    }
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<int, float, int> b(std::in_place_index<2>, 33 - 4);
        b = a;
        REQUIRE(b.index() == 2);  // b's contained value is directly assigned from a's contained value, no alternative changed
    }
}

TEST_CASE("flexible move assign") {
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<int, float> b = 3.14f;
        REQUIRE_NOTHROW(b = std::move(a));
    }
    {
        yk::rvariant<int> a = 42;
        yk::rvariant<int, float, int> b(std::in_place_index<2>, 33 - 4);
        b = std::move(a);
        REQUIRE(b.index() == 2);  // b's contained value is directly assigned from a's contained value, no alternative changed
    }
}

TEST_CASE("generic assign") {
    {
        yk::rvariant<int, float> a = 42;
        // same alternative
        a = 33 - 4;
        REQUIRE(a.index() == 0);
        // different alternative
        a = 3.14f;
        REQUIRE(a.index() == 1);
    }
}

TEST_CASE("emplace") {
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

TEST_CASE("raw get") {
    yk::rvariant<int, float> var = 42;
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::raw_get<0>(var)), yk::detail::alternative<0, int>&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::raw_get<0>(std::move(var))), yk::detail::alternative<0, int>&&>);
}

TEST_CASE("get") {
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(yk::get<0>(var) == 42);
        REQUIRE(yk::get<0>(std::as_const(var)) == 42);
        REQUIRE(yk::get<0>(std::move(var)) == 42);
        REQUIRE(yk::get<0>(std::move(std::as_const(var))) == 42);
    }
}

TEST_CASE("swap") {
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

TEST_CASE("holds alternative") {
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(yk::holds_alternative<int>(var));
    }
}

TEST_CASE("recursive wrapper") {
    yk::recursive_wrapper<int> a(42);
    REQUIRE_FALSE(a.valueless_after_move());

    {
        auto f = [](yk::rvariant<yk::recursive_wrapper<int>>) {};
        f(42);
    }
    {
        auto f = [](yk::rvariant<yk::recursive_wrapper<int>, double> var) { return yk::holds_alternative<double>(var); };
        REQUIRE(f(3.14f));
    }
    {
        auto f = [](yk::rvariant<yk::recursive_wrapper<double>, int> var) { return yk::holds_alternative<double>(var); };
        REQUIRE(f(3.14f));
    }
    {
        auto f = [](yk::rvariant<yk::recursive_wrapper<double>, int> var) { return yk::holds_alternative<double>(var); };
        REQUIRE(f(3.14));
    }
    {
        auto f = [](yk::rvariant<yk::recursive_wrapper<double>, int> var) { return yk::holds_alternative<int>(var); };
        REQUIRE(f(3));
    }
    {
        auto f = [](yk::rvariant<yk::recursive_wrapper<double>, int> var) { return yk::holds_alternative<double>(var); };
        REQUIRE(f(yk::recursive_wrapper<double>(3.14)));
    }
}

TEST_CASE("unwrap recursive") {
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<int&>())), int&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<int&&>())), int&&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<int const&>())), int const&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<int const&&>())), int const&&>);

    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<yk::recursive_wrapper<int>&>())), int&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<yk::recursive_wrapper<int>&&>())), int&&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<yk::recursive_wrapper<int> const&>())), int const&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::unwrap_recursive(std::declval<yk::recursive_wrapper<int> const&&>())), int const&&>);
}
