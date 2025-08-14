// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "rvariant_test.hpp"

#include "yk/rvariant/rvariant.hpp"
#include "yk/rvariant/recursive_wrapper.hpp"
#include "yk/rvariant/variant_helper.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <string>
#include <type_traits>
#include <utility>

namespace unit_test {

namespace {

template<std::size_t I>
struct Index
{
    constexpr ~Index() noexcept  // NOLINT(modernize-use-equals-default)
    {
        // non-trivial
    }

    int value = I * 2;
};

template<std::size_t Size, class Seq = std::make_index_sequence<Size>>
struct many_V_impl;

template<std::size_t Size, std::size_t... Is>
struct many_V_impl<Size, std::index_sequence<Is...>>
{
    using type = yk::rvariant<Index<Is>...>;
};

template<std::size_t Size>
using many_V_t = typename many_V_impl<Size>::type;

} // anonymous

TEST_CASE("raw_get", "[detail]")
{
    using yk::detail::raw_get;
    using Storage = yk::detail::make_variadic_union_t<int>;

    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage&>())), int&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage const&>())), int const&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage&&>())), int&&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage const&&>())), int const&&>);

    {
        using V = many_V_t<1>;
        constexpr V v(std::in_place_index<0>);
        STATIC_CHECK(yk::get<0>(v).value == 0 * 2);
    }

    {
        using V = many_V_t<33>;
        STATIC_CHECK(yk::get< 0>(V(std::in_place_index< 0>)).value ==  0 * 2);
        STATIC_CHECK(yk::get< 1>(V(std::in_place_index< 1>)).value ==  1 * 2);
        STATIC_CHECK(yk::get< 2>(V(std::in_place_index< 2>)).value ==  2 * 2);
        STATIC_CHECK(yk::get< 3>(V(std::in_place_index< 3>)).value ==  3 * 2);
        STATIC_CHECK(yk::get< 4>(V(std::in_place_index< 4>)).value ==  4 * 2);
        STATIC_CHECK(yk::get< 5>(V(std::in_place_index< 5>)).value ==  5 * 2);
        STATIC_CHECK(yk::get< 6>(V(std::in_place_index< 6>)).value ==  6 * 2);
        STATIC_CHECK(yk::get< 7>(V(std::in_place_index< 7>)).value ==  7 * 2);
        STATIC_CHECK(yk::get< 8>(V(std::in_place_index< 8>)).value ==  8 * 2);
        STATIC_CHECK(yk::get< 9>(V(std::in_place_index< 9>)).value ==  9 * 2);

        STATIC_CHECK(yk::get<10>(V(std::in_place_index<10>)).value == 10 * 2);
        STATIC_CHECK(yk::get<11>(V(std::in_place_index<11>)).value == 11 * 2);
        STATIC_CHECK(yk::get<12>(V(std::in_place_index<12>)).value == 12 * 2);
        STATIC_CHECK(yk::get<13>(V(std::in_place_index<13>)).value == 13 * 2);
        STATIC_CHECK(yk::get<14>(V(std::in_place_index<14>)).value == 14 * 2);
        STATIC_CHECK(yk::get<15>(V(std::in_place_index<15>)).value == 15 * 2);
        STATIC_CHECK(yk::get<16>(V(std::in_place_index<16>)).value == 16 * 2);
        STATIC_CHECK(yk::get<17>(V(std::in_place_index<17>)).value == 17 * 2);
        STATIC_CHECK(yk::get<18>(V(std::in_place_index<18>)).value == 18 * 2);
        STATIC_CHECK(yk::get<19>(V(std::in_place_index<19>)).value == 19 * 2);

        STATIC_CHECK(yk::get<20>(V(std::in_place_index<20>)).value == 20 * 2);
        STATIC_CHECK(yk::get<21>(V(std::in_place_index<21>)).value == 21 * 2);
        STATIC_CHECK(yk::get<22>(V(std::in_place_index<22>)).value == 22 * 2);
        STATIC_CHECK(yk::get<23>(V(std::in_place_index<23>)).value == 23 * 2);
        STATIC_CHECK(yk::get<24>(V(std::in_place_index<24>)).value == 24 * 2);
        STATIC_CHECK(yk::get<25>(V(std::in_place_index<25>)).value == 25 * 2);
        STATIC_CHECK(yk::get<26>(V(std::in_place_index<26>)).value == 26 * 2);
        STATIC_CHECK(yk::get<27>(V(std::in_place_index<27>)).value == 27 * 2);
        STATIC_CHECK(yk::get<28>(V(std::in_place_index<28>)).value == 28 * 2);
        STATIC_CHECK(yk::get<29>(V(std::in_place_index<29>)).value == 29 * 2);

        STATIC_CHECK(yk::get<30>(V(std::in_place_index<30>)).value == 30 * 2);
        STATIC_CHECK(yk::get<31>(V(std::in_place_index<31>)).value == 31 * 2);
        STATIC_CHECK(yk::get<32>(V(std::in_place_index<32>)).value == 32 * 2);
    }
    {
        using V = many_V_t<66>;

        STATIC_CHECK(yk::get<62>(V(std::in_place_index<62>)).value == 62 * 2);
        STATIC_CHECK(yk::get<63>(V(std::in_place_index<63>)).value == 63 * 2);
        STATIC_CHECK(yk::get<64>(V(std::in_place_index<64>)).value == 64 * 2);
        STATIC_CHECK(yk::get<65>(V(std::in_place_index<65>)).value == 65 * 2);
    }
}

// Required for suppressing std::move(const&)
// NOLINTBEGIN(performance-move-const-arg)
TEST_CASE("get")
{
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
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        REQUIRE_THROWS(yk::get<int>(valueless));
        REQUIRE_THROWS(yk::get<MC_Thrower>(valueless));
    }
}

TEST_CASE("get", "[wrapper]")
{
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
}
// NOLINTEND(performance-move-const-arg)

TEST_CASE("get_if")
{
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(*yk::get_if<0>(&var) == 42);
        REQUIRE(*yk::get_if<0>(&std::as_const(var)) == 42);
        REQUIRE(yk::get_if<1>(&var) == nullptr);
        REQUIRE(yk::get_if<1>(&std::as_const(var)) == nullptr);
    }
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(*yk::get_if<int>(&var) == 42);
        REQUIRE(*yk::get_if<int>(&std::as_const(var)) == 42);
        REQUIRE(yk::get_if<float>(&var) == nullptr);
        REQUIRE(yk::get_if<float>(&std::as_const(var)) == nullptr);
    }
    {
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        REQUIRE(yk::get_if<0>(&valueless) == nullptr);
        REQUIRE(yk::get_if<1>(&valueless) == nullptr);
        REQUIRE(yk::get_if<int>(&valueless) == nullptr);
        REQUIRE(yk::get_if<MC_Thrower>(&valueless) == nullptr);
    }
}

TEST_CASE("get_if", "[wrapper]")
{
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(*yk::get_if<0>(&var) == 42);
        REQUIRE(*yk::get_if<0>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(*yk::get_if<int>(&var) == 42);
        REQUIRE(*yk::get_if<int>(&std::as_const(var)) == 42);
    }
}

namespace {

template<bool IsNoexcept, class R, class F, class Visitor, class Storage>
constexpr bool is_noexcept_invocable_r_v = std::conditional_t<
    IsNoexcept,
    std::conjunction<std::is_invocable_r<R, F, Visitor, Storage>, std::is_nothrow_invocable_r<R, F, Visitor, Storage>>,
    std::conjunction<std::is_invocable_r<R, F, Visitor, Storage>, std::negation<std::is_nothrow_invocable_r<R, F, Visitor, Storage>>>
>::value;

} // anonymous

TEST_CASE("raw_visit", "[detail]")
{
    using yk::detail::raw_visit_dispatch;
    using yk::detail::raw_visit_table;
    using yk::detail::raw_visit_noexcept_all;
    using Storage = yk::detail::make_variadic_union_t<int, double>;
    using yk::detail::valueless_t;

    // NOLINTBEGIN(cppcoreguidelines-rvalue-reference-param-not-moved)

    // invoke(overload{noexcept(true), noexcept(true)}, &)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<std::variant_npos>, valueless_t&) noexcept { return true; },
            [](std::in_place_index_t<0>, int&) noexcept(true) { return true; },
            [](std::in_place_index_t<1>, double&) noexcept(true) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(raw_visit_noexcept_all<Visitor const&, Storage&>);

        constexpr auto const& table = raw_visit_table<Visitor const&, Storage&>::table;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[std::variant_npos + 1]), Visitor const&, Storage&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[0 + 1]), Visitor const&, Storage&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[1 + 1]), Visitor const&, Storage&>);
    }
    // invoke(overload{noexcept(true), noexcept(false)}, &)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<std::variant_npos>, valueless_t&) noexcept { return true; },
            [](std::in_place_index_t<0>, int&) noexcept(false) { return true; },
            [](std::in_place_index_t<1>, double&) noexcept(false) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(!raw_visit_noexcept_all<bool, Visitor const&, Storage&>);

        constexpr auto const& table = raw_visit_table<Visitor const&, Storage&>::table;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[std::variant_npos + 1]), Visitor const&, Storage&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[0 + 1]), Visitor const&, Storage&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[1 + 1]), Visitor const&, Storage&>);
    }

    // invoke(overload{noexcept(true), noexcept(true)}, const&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<std::variant_npos>, valueless_t const&) noexcept { return true; },
            [](std::in_place_index_t<0>, int const&) noexcept(true) { return true; },
            [](std::in_place_index_t<1>, double const&) noexcept(true) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(raw_visit_noexcept_all<Visitor const&, Storage const&>);

        constexpr auto const& table = raw_visit_table<Visitor const&, Storage const&>::table;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[std::variant_npos + 1]), Visitor const&, Storage const&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[0 + 1]), Visitor const&, Storage const&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[1 + 1]), Visitor const&, Storage const&>);
    }
    // invoke(overload{noexcept(true), noexcept(false)}, const&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<std::variant_npos>, valueless_t const&) noexcept { return true; },
            [](std::in_place_index_t<0>, int const&) noexcept(false) { return true; },
            [](std::in_place_index_t<1>, double const&) noexcept(false) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(!raw_visit_noexcept_all<Visitor const&, Storage const&>);

        constexpr auto const& table = raw_visit_table<Visitor const&, Storage const&>::table;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[std::variant_npos + 1]), Visitor const&, Storage const&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[0 + 1]), Visitor const&, Storage const&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[1 + 1]), Visitor const&, Storage const&>);
    }

    // invoke(overload{noexcept(true), noexcept(true)}, &&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<std::variant_npos>, valueless_t&&) noexcept { return true; },
            [](std::in_place_index_t<0>, int&&) noexcept(true) { return true; },
            [](std::in_place_index_t<1>, double&&) noexcept(true) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(raw_visit_noexcept_all<Visitor const&, Storage&&>);

        constexpr auto const& table = raw_visit_table<Visitor const&, Storage&&>::table;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[std::variant_npos + 1]), Visitor const&, Storage&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[0 + 1]), Visitor const&, Storage&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[1 + 1]), Visitor const&, Storage&&>);
    }
    // invoke(overload{noexcept(true), noexcept(false)}, &&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<std::variant_npos>, valueless_t&&) noexcept { return true; },
            [](std::in_place_index_t<0>, int&&) noexcept(false) { return true; },
            [](std::in_place_index_t<1>, double&&) noexcept(false) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(!raw_visit_noexcept_all<Visitor const&, Storage&&>);

        constexpr auto const& table = raw_visit_table<Visitor const&, Storage&&>::table;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[std::variant_npos + 1]), Visitor const&, Storage&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[0 + 1]), Visitor const&, Storage&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[1 + 1]), Visitor const&, Storage&&>);
    }

    // invoke(overload{noexcept(true), noexcept(true)}, const&&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<std::variant_npos>, valueless_t const&&) noexcept { return true; },
            [](std::in_place_index_t<0>, int const&&) noexcept(true) { return true; },
            [](std::in_place_index_t<1>, double const&&) noexcept(true) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(raw_visit_noexcept_all<Visitor const&, Storage const&&>);

        constexpr auto const& table = raw_visit_table<Visitor const&, Storage const&&>::table;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[std::variant_npos + 1]), Visitor const&, Storage const&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[0 + 1]), Visitor const&, Storage const&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[1 + 1]), Visitor const&, Storage const&&>);
    }
    // invoke(overload{noexcept(true), noexcept(false)}, const&&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<std::variant_npos>, valueless_t const&&) noexcept { return true; },
            [](std::in_place_index_t<0>, int const&&) noexcept(false) { return true; },
            [](std::in_place_index_t<1>, double const&&) noexcept(false) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(!raw_visit_noexcept_all<Visitor const&, Storage const&&>);

        constexpr auto const& table = raw_visit_table<Visitor const&, Storage const&&>::table;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[std::variant_npos + 1]), Visitor const&, Storage const&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[0 + 1]), Visitor const&, Storage const&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[1 + 1]), Visitor const&, Storage const&&>);
    }

    // NOLINTEND(cppcoreguidelines-rvalue-reference-param-not-moved)
}

TEST_CASE("flat_index", "[detail]")
{
    using yk::detail::flat_index;
    using std::index_sequence;
    {
        STATIC_REQUIRE(flat_index<index_sequence<1>, true>::get(0) == 0);

        STATIC_REQUIRE(flat_index<index_sequence<2>, true>::get(0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2>, true>::get(1) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<1, 1>, true, true>::get(0, 0) == 0);

        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, true, true>::get(0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, true, true>::get(1, 0) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, true, true>::get(0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, true, true>::get(0, 1) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, true, true>::get(0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, true, true>::get(0, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, true, true>::get(1, 0) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, true, true>::get(1, 1) == 3);

        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, true, true>::get(0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, true, true>::get(0, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, true, true>::get(0, 2) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, true, true>::get(1, 0) == 3);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, true, true>::get(1, 1) == 4);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, true, true>::get(1, 2) == 5);

        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 0, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 0, 2) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 0, 3) == 3);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 1, 0) == 4);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 1, 1) == 5);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 1, 2) == 6);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 1, 3) == 7);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 2, 0) == 8);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 2, 1) == 9);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 2, 2) == 10);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(0, 2, 3) == 11);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 0, 0) == 12);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 0, 1) == 13);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 0, 2) == 14);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 0, 3) == 15);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 1, 0) == 16);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 1, 1) == 17);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 1, 2) == 18);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 1, 3) == 19);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 2, 0) == 20);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 2, 1) == 21);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 2, 2) == 22);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, true, true, true>::get(1, 2, 3) == 23);
    }
    {
        STATIC_REQUIRE(flat_index<index_sequence<1>, false>::get(-1) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<1>, false>::get( 0) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<2>, false>::get(-1) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2>, false>::get( 0) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2>, false>::get( 1) == 2);

        STATIC_REQUIRE(flat_index<index_sequence<1, 1>, false, true>::get(-1, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<1, 1>, false, true>::get( 0, 0) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, false, true>::get(-1, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, false, true>::get( 0, 0) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, false, true>::get( 1, 0) == 2);

        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, false, true>::get(-1, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, false, true>::get(-1, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, false, true>::get( 0, 0) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, false, true>::get( 0, 1) == 3);

        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, false, true>::get(-1,  0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, false, true>::get(-1,  1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, false, true>::get( 0,  0) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, false, true>::get( 0,  1) == 3);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, false, true>::get( 1,  0) == 4);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, false, true>::get( 1,  1) == 5);

        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get(-1, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get(-1, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get(-1, 2) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get( 0, 0) == 3);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get( 0, 1) == 4);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get( 0, 2) == 5);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get( 1, 0) == 6);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get( 1, 1) == 7);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, false, true>::get( 1, 2) == 8);

        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get(-1, 0, -1) ==  0);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get(-1, 0,  0) ==  1);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get(-1, 0,  1) ==  2);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get(-1, 1, -1) ==  3);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get(-1, 1,  0) ==  4);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get(-1, 1,  1) ==  5);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 0, 0, -1) ==  6);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 0, 0,  0) ==  7);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 0, 0,  1) ==  8);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 0, 1, -1) ==  9);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 0, 1,  0) == 10);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 0, 1,  1) == 11);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 1, 0, -1) == 12);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 1, 0,  0) == 13);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 1, 0,  1) == 14);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 1, 1, -1) == 15);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 1, 1,  0) == 16);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 1, 1,  1) == 17);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 2, 0, -1) == 18);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 2, 0,  0) == 19);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 2, 0,  1) == 20);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 2, 1, -1) == 21);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 2, 1,  0) == 22);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, false, true, false>::get( 2, 1,  1) == 23);
    }
}

namespace {

template<class T>
struct strong
{
    T value;
};

namespace not_a_variant_ADL {

template<class... Ts>
struct DerivedVariant : yk::rvariant<Ts...>
{
    using DerivedVariant::rvariant::rvariant;
};

template<class... Ts>
struct not_a_variant {};

template<class R, class Foo, class... Bar>
    requires (!std::disjunction_v<yk::core::is_ttp_specialization_of<std::remove_cvref_t<Bar>, DerivedVariant>...>)
constexpr R visit(Foo&&, Bar&&...)
{
    return R{"not_a_variant"};
}

template<class Foo, class... Bar>
    requires (!std::disjunction_v<yk::core::is_ttp_specialization_of<std::remove_cvref_t<Bar>, DerivedVariant>...>)
constexpr decltype(auto) visit(Foo&&, Bar&&...)
{
    return std::string_view{"not_a_variant"};
}

} // not_a_variant_ADL

namespace SFINAE_context {

using ::yk::visit;
using not_a_variant_ADL::visit;

template<class Visitor, class Variant, class Enabled = void>
struct overload_resolvable : std::false_type {};

template<class Visitor, class Variant>
struct overload_resolvable<
    Visitor,
    Variant,
    std::void_t<decltype(
        visit<std::string_view>(std::declval<Visitor>(), std::declval<Variant>()
    ))>
> : std::true_type {};

} // SFINAE_context_ns

} // anonymous

TEST_CASE("visit (Constraints)")
{
    [[maybe_unused]] constexpr auto vis = yk::overloaded{
        [](int const&) -> std::string_view { return "variant"; },
        [](float const&) -> std::string_view { return "variant"; },
    };
    using Visitor = decltype(vis);

    [[maybe_unused]] constexpr auto different_R_vis = yk::overloaded{
        [](int const&) -> std::string_view { return "variant"; },
        [](float const&) -> std::string /* different type */ { return "variant"; },
    };
    using DifferentRVisitor = decltype(different_R_vis);

    STATIC_REQUIRE(std::is_invocable_v<Visitor, int>);
    STATIC_REQUIRE(std::is_invocable_v<Visitor, float>);
    STATIC_REQUIRE(!std::is_invocable_v<Visitor, double>); // ambiguous

    // for `visit(...)`
    {
        using IntChecker = yk::detail::visit_check_impl<std::string_view, Visitor, yk::core::type_list<int>>;
        using DoubleChecker = yk::detail::visit_check_impl<std::string_view, Visitor, yk::core::type_list<double>>;

        STATIC_REQUIRE(IntChecker::accepts_all_alternatives);
        STATIC_REQUIRE(IntChecker::value);
        STATIC_REQUIRE(!DoubleChecker::accepts_all_alternatives);
        STATIC_REQUIRE(!DoubleChecker::value);

        {
            using Check = yk::detail::visit_check<std::string_view, Visitor, yk::rvariant<int, float>&&>;
            STATIC_REQUIRE(Check::accepts_all_alternatives);
            STATIC_REQUIRE(Check::same_return_type);
            STATIC_REQUIRE(Check::value);
        }
        {
            using Check = yk::detail::visit_check<std::string_view, DifferentRVisitor, yk::rvariant<int, float>&&>;
            STATIC_REQUIRE(Check::accepts_all_alternatives);
            STATIC_REQUIRE(!Check::same_return_type);
            STATIC_REQUIRE(!Check::value);
        }
        {
            using Check = yk::detail::visit_check<std::string_view, Visitor, yk::rvariant<int, double>&&>;
            STATIC_REQUIRE(!Check::accepts_all_alternatives);
            STATIC_REQUIRE(!Check::value);
        }
    }
    // for `visit<R>(...)`
    {
        using IntChecker = yk::detail::visit_R_check_impl<std::string_view, Visitor, yk::core::type_list<int>>;
        using DoubleChecker = yk::detail::visit_R_check_impl<std::string_view, Visitor, yk::core::type_list<double>>;

        STATIC_REQUIRE(IntChecker::accepts_all_alternatives);
        STATIC_REQUIRE(IntChecker::return_type_convertible_to_R);
        STATIC_REQUIRE(IntChecker::value);
        STATIC_REQUIRE(!DoubleChecker::accepts_all_alternatives);
        STATIC_REQUIRE(!DoubleChecker::value);

        {
            using Check = yk::detail::visit_R_check<std::string_view, Visitor, yk::rvariant<int, float>&&>;
            STATIC_REQUIRE(Check::accepts_all_alternatives);
            STATIC_REQUIRE(Check::return_type_convertible_to_R);
            STATIC_REQUIRE(Check::value);
        }
        {
            using Check = yk::detail::visit_R_check<std::string, DifferentRVisitor, yk::rvariant<int, float>&&>;
            STATIC_REQUIRE(Check::accepts_all_alternatives);
            STATIC_REQUIRE(!Check::return_type_convertible_to_R);
            STATIC_REQUIRE(!Check::value);
        }
        {
            using Check = yk::detail::visit_R_check<std::string_view, Visitor, yk::rvariant<int, double>&&>;
            STATIC_REQUIRE(!Check::accepts_all_alternatives);
            STATIC_REQUIRE(!Check::value);
        }
    }

    {
        // Asserts the "Constraints:" is implemented correctly
        // https://eel.is/c++draft/variant.visit#2
        STATIC_REQUIRE(requires {
            requires std::same_as<std::true_type, SFINAE_context::overload_resolvable<Visitor, not_a_variant_ADL::not_a_variant<int, float>>::type>;
        });

        using std::visit;
        STATIC_CHECK(visit<std::string_view>(vis, not_a_variant_ADL::not_a_variant<int, float>{}) == "not_a_variant");
        STATIC_CHECK(visit<std::string_view>(vis, std::variant<int, float>{}) == "variant");
    }
    {
        // Asserts the "Constraints:" is implemented correctly
        // https://eel.is/c++draft/variant.visit#2
        STATIC_REQUIRE(requires {
            requires std::same_as<std::true_type, SFINAE_context::overload_resolvable<Visitor, not_a_variant_ADL::not_a_variant<int, float>>::type>;
        });

        using ::yk::visit;
        STATIC_CHECK(visit<std::string_view>(vis, not_a_variant_ADL::not_a_variant<int, float>{}) == "not_a_variant");
        STATIC_CHECK(visit<std::string_view>(vis, yk::rvariant<int, float>{}) == "variant");
        STATIC_CHECK(visit(vis, not_a_variant_ADL::not_a_variant<int, float>{}) == "not_a_variant");
        STATIC_CHECK(visit(vis, yk::rvariant<int, float>{}) == "variant");
        STATIC_CHECK(yk::rvariant<int, float>{}.visit<std::string_view>(vis) == "variant");
        STATIC_CHECK(yk::rvariant<int, float>{}.visit(vis) == "variant");

        // Asserts as-variant is working
        STATIC_CHECK(visit<std::string_view>(vis, not_a_variant_ADL::DerivedVariant<int, float>{42}) == "variant");
        STATIC_CHECK(visit(vis, not_a_variant_ADL::DerivedVariant<int, float>{42}) == "variant");
        STATIC_CHECK(not_a_variant_ADL::DerivedVariant<int, float>{42}.visit<std::string_view>(vis) == "variant");
        STATIC_CHECK(not_a_variant_ADL::DerivedVariant<int, float>{42}.visit(vis) == "variant");
    }
    {
        // Not permitted (hard error); std::string_view -> std::string is not implicitly convertible
        //STATIC_CHECK(std::visit<std::string>(different_R_vis, std::variant<int>{}) == "variant");
        //STATIC_CHECK(yk::visit<std::string>(different_R_vis, yk::rvariant<int>{}) == "variant");
    }
    {
        // Implicit cast from different types shall be permitted as per INVOKE<R>(...)
        STATIC_CHECK(std::visit<std::string_view>(different_R_vis, std::variant<int, float>{}) == "variant");
        STATIC_CHECK(yk::visit<std::string_view>(different_R_vis, yk::rvariant<int, float>{}) == "variant");
    }
    {
        // Case for T0 leading to ill-formed invocation.
        // In such case, a naive call to `std::invoke_result_t` will result in
        // a hard error. We try to avoid that situation, correctly engaging
        // `static_assert` error instead of numerous hard errors.

        // ill-formed (not even a `static_assert` error on MSVC)
        //std::visit<std::string_view>(vis, std::variant<double>{});

        // expected static_assert error
        //yk::visit<std::string_view>(vis, yk::rvariant<double>{});
    }

    // TODO: add test for this
    //std::visit(vis, std::variant<double>{}); // no matching overload
    //yk::visit(vis, yk::rvariant<double>{}); // no matching overload
    // Make sure "no matching overload" is SFINAE-friendly.
}

TEST_CASE("visit")
{
    using SI = strong<int>;
    using SD = strong<double>;
    using SC = strong<char>;
    using SW = strong<wchar_t>;

    {
        constexpr auto vis = yk::overloaded{
            [](SI const&) { return 0; },
            [](SD const&) { return 1; },
        };
        STATIC_CHECK(std::visit<int>(vis, std::variant<SI, SD>{SI{}}) == 0);
        STATIC_CHECK(std::visit<int>(vis, std::variant<SI, SD>{SD{}}) == 1);

        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{SI{}}) == 0);
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{SD{}}) == 1);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<SI, SD>{SI{}}) == 0);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<SI, SD>{SD{}}) == 1);

        STATIC_CHECK(yk::rvariant<SI, SD>{SI{}}.visit<int>(vis) == 0);
        STATIC_CHECK(yk::rvariant<SI, SD>{SD{}}.visit<int>(vis) == 1);
        STATIC_CHECK(yk::rvariant<SI, SD>{SI{}}.visit(vis) == 0);
        STATIC_CHECK(yk::rvariant<SI, SD>{SD{}}.visit(vis) == 1);
    }
    {
        constexpr auto vis = yk::overloaded{
            [](SI const&, SC const&) { return 0; },
            [](SI const&, SW const&) { return 1; },
            [](SD const&, SC const&) { return 2; },
            [](SD const&, SW const&) { return 3; },
        };
        STATIC_CHECK(std::visit<int>(vis, std::variant<SI, SD>{SI{}}, std::variant<SC, SW>{SC{}}) == 0);
        STATIC_CHECK(std::visit<int>(vis, std::variant<SI, SD>{SI{}}, std::variant<SC, SW>{SW{}}) == 1);
        STATIC_CHECK(std::visit<int>(vis, std::variant<SI, SD>{SD{}}, std::variant<SC, SW>{SC{}}) == 2);
        STATIC_CHECK(std::visit<int>(vis, std::variant<SI, SD>{SD{}}, std::variant<SC, SW>{SW{}}) == 3);

        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{SI{}}, yk::rvariant<SC, SW>{SC{}}) == 0);
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{SI{}}, yk::rvariant<SC, SW>{SW{}}) == 1);
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{SD{}}, yk::rvariant<SC, SW>{SC{}}) == 2);
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{SD{}}, yk::rvariant<SC, SW>{SW{}}) == 3);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<SI, SD>{SI{}}, yk::rvariant<SC, SW>{SC{}}) == 0);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<SI, SD>{SI{}}, yk::rvariant<SC, SW>{SW{}}) == 1);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<SI, SD>{SD{}}, yk::rvariant<SC, SW>{SC{}}) == 2);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<SI, SD>{SD{}}, yk::rvariant<SC, SW>{SW{}}) == 3);
    }

    {
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        auto const vis = [](auto&&) {};
        CHECK_THROWS(yk::visit<void>(vis, valueless));
        CHECK_THROWS(yk::visit(vis, valueless));
        CHECK_THROWS(valueless.visit<void>(vis));
        CHECK_THROWS(valueless.visit(vis));
    }
    {
        yk::rvariant<int> never_valueless;
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        auto const vis = [](auto&&, auto&&, auto&&) {};
        CHECK_THROWS(yk::visit<void>(vis, never_valueless, valueless, never_valueless));
        CHECK_THROWS(yk::visit(vis, never_valueless, valueless, never_valueless));
        CHECK_THROWS(yk::visit<void>(vis, valueless, never_valueless, valueless));
        CHECK_THROWS(yk::visit(vis, valueless, never_valueless, valueless));
    }
}

// Equivalent to the "visit" test case, except that `SI` is wrapped
TEST_CASE("visit", "[wrapper]")
{
    using SI = strong<int>;
    using SD = strong<double>;
    using SC = strong<char>;
    using SW = strong<wchar_t>;

    {
        constexpr auto vis = yk::overloaded{
            [](SI /* unwrapped */ const&) { return 0; },
            [](SD const&) { return 1; },
        };
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SI{}}) == 0);
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SD{}}) == 1);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SI{}}) == 0);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SD{}}) == 1);

        STATIC_CHECK(yk::rvariant<yk::recursive_wrapper<SI>, SD>{SI{}}.visit<int>(vis) == 0);
        STATIC_CHECK(yk::rvariant<yk::recursive_wrapper<SI>, SD>{SD{}}.visit<int>(vis) == 1);
        STATIC_CHECK(yk::rvariant<yk::recursive_wrapper<SI>, SD>{SI{}}.visit(vis) == 0);
        STATIC_CHECK(yk::rvariant<yk::recursive_wrapper<SI>, SD>{SD{}}.visit(vis) == 1);
    }
    {
        constexpr auto vis = yk::overloaded{
            [](SI /* unwrapped */ const&, SC const&) { return 0; },
            [](SI /* unwrapped */ const&, SW const&) { return 1; },
            [](SD const&, SC const&) { return 2; },
            [](SD const&, SW const&) { return 3; },
        };
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SI{}}, yk::rvariant<SC, SW>{SC{}}) == 0);
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SI{}}, yk::rvariant<SC, SW>{SW{}}) == 1);
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SD{}}, yk::rvariant<SC, SW>{SC{}}) == 2);
        STATIC_CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SD{}}, yk::rvariant<SC, SW>{SW{}}) == 3);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SI{}}, yk::rvariant<SC, SW>{SC{}}) == 0);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SI{}}, yk::rvariant<SC, SW>{SW{}}) == 1);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SD{}}, yk::rvariant<SC, SW>{SC{}}) == 2);
        STATIC_CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{SD{}}, yk::rvariant<SC, SW>{SW{}}) == 3);
    }
}

} // unit_test
