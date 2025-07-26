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

TEST_CASE("raw_get", "[detail]")
{
    using yk::detail::raw_get;
    using Storage = yk::detail::make_variadic_union_t<int>;

    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage&>())), int&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage const&>())), int const&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage&&>())), int&&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage const&&>())), int const&&>);
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
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        REQUIRE_THROWS(yk::get<int>(valueless));
        REQUIRE_THROWS(yk::get<MoveThrows>(valueless));
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
        REQUIRE(*yk::get<0>(&var) == 42);
        REQUIRE(*yk::get<0>(&std::as_const(var)) == 42);
        REQUIRE(yk::get<1>(&var) == nullptr);
        REQUIRE(yk::get<1>(&std::as_const(var)) == nullptr);
    }
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(*yk::get_if<int>(&var) == 42);
        REQUIRE(*yk::get_if<int>(&std::as_const(var)) == 42);
        REQUIRE(yk::get_if<float>(&var) == nullptr);
        REQUIRE(yk::get_if<float>(&std::as_const(var)) == nullptr);
    }
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(*yk::get<int>(&var) == 42);
        REQUIRE(*yk::get<int>(&std::as_const(var)) == 42);
        REQUIRE(yk::get<float>(&var) == nullptr);
        REQUIRE(yk::get<float>(&std::as_const(var)) == nullptr);
    }
    {
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        REQUIRE(yk::get_if<0>(&valueless) == nullptr);
        REQUIRE(yk::get_if<1>(&valueless) == nullptr);
        REQUIRE(yk::get_if<int>(&valueless) == nullptr);
        REQUIRE(yk::get_if<MoveThrows>(&valueless) == nullptr);
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
        REQUIRE(*yk::get<0>(&var) == 42);
        REQUIRE(*yk::get<0>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(*yk::get_if<int>(&var) == 42);
        REQUIRE(*yk::get_if<int>(&std::as_const(var)) == 42);
    }
    {
        yk::rvariant<yk::recursive_wrapper<int>, float> var = 42;
        REQUIRE(*yk::get<int>(&var) == 42);
        REQUIRE(*yk::get<int>(&std::as_const(var)) == 42);
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
    using yk::detail::raw_visit_dispatch_table;
    using yk::detail::raw_visit_noexcept;
    using Storage = yk::detail::make_variadic_union_t<int, double>;
    using yk::detail::valueless_t;
    using yk::detail::variant_npos;

    // NOLINTBEGIN(cppcoreguidelines-rvalue-reference-param-not-moved)

    // invoke(overload{noexcept(true), noexcept(true)}, &)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<variant_npos>, valueless_t&) noexcept { return true; },
            [](std::in_place_index_t<0>, int&) noexcept(true) { return true; },
            [](std::in_place_index_t<1>, double&) noexcept(true) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(raw_visit_noexcept<Visitor const&, Storage&>);

        constexpr auto const& table = raw_visit_dispatch_table<Visitor const&, Storage&>::value;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[variant_npos + 1]), Visitor const&, Storage&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[0 + 1]), Visitor const&, Storage&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[1 + 1]), Visitor const&, Storage&>);
    }
    // invoke(overload{noexcept(true), noexcept(false)}, &)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<variant_npos>, valueless_t&) noexcept { return true; },
            [](std::in_place_index_t<0>, int&) noexcept(false) { return true; },
            [](std::in_place_index_t<1>, double&) noexcept(false) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(!raw_visit_noexcept<Visitor const&, Storage&>);

        constexpr auto const& table = raw_visit_dispatch_table<Visitor const&, Storage&>::value;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[variant_npos + 1]), Visitor const&, Storage&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[0 + 1]), Visitor const&, Storage&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[1 + 1]), Visitor const&, Storage&>);
    }

    // invoke(overload{noexcept(true), noexcept(true)}, const&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<variant_npos>, valueless_t const&) noexcept { return true; },
            [](std::in_place_index_t<0>, int const&) noexcept(true) { return true; },
            [](std::in_place_index_t<1>, double const&) noexcept(true) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(raw_visit_noexcept<Visitor const&, Storage const&>);

        constexpr auto const& table = raw_visit_dispatch_table<Visitor const&, Storage const&>::value;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[variant_npos + 1]), Visitor const&, Storage const&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[0 + 1]), Visitor const&, Storage const&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[1 + 1]), Visitor const&, Storage const&>);
    }
    // invoke(overload{noexcept(true), noexcept(false)}, const&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<variant_npos>, valueless_t const&) noexcept { return true; },
            [](std::in_place_index_t<0>, int const&) noexcept(false) { return true; },
            [](std::in_place_index_t<1>, double const&) noexcept(false) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(!raw_visit_noexcept<Visitor const&, Storage const&>);

        constexpr auto const& table = raw_visit_dispatch_table<Visitor const&, Storage const&>::value;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[variant_npos + 1]), Visitor const&, Storage const&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[0 + 1]), Visitor const&, Storage const&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[1 + 1]), Visitor const&, Storage const&>);
    }

    // invoke(overload{noexcept(true), noexcept(true)}, &&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<variant_npos>, valueless_t&&) noexcept { return true; },
            [](std::in_place_index_t<0>, int&&) noexcept(true) { return true; },
            [](std::in_place_index_t<1>, double&&) noexcept(true) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(raw_visit_noexcept<Visitor const&, Storage&&>);

        constexpr auto const& table = raw_visit_dispatch_table<Visitor const&, Storage&&>::value;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[variant_npos + 1]), Visitor const&, Storage&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[0 + 1]), Visitor const&, Storage&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[1 + 1]), Visitor const&, Storage&&>);
    }
    // invoke(overload{noexcept(true), noexcept(false)}, &&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<variant_npos>, valueless_t&&) noexcept { return true; },
            [](std::in_place_index_t<0>, int&&) noexcept(false) { return true; },
            [](std::in_place_index_t<1>, double&&) noexcept(false) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(!raw_visit_noexcept<Visitor const&, Storage&&>);

        constexpr auto const& table = raw_visit_dispatch_table<Visitor const&, Storage&&>::value;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[variant_npos + 1]), Visitor const&, Storage&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[0 + 1]), Visitor const&, Storage&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[1 + 1]), Visitor const&, Storage&&>);
    }

    // invoke(overload{noexcept(true), noexcept(true)}, const&&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<variant_npos>, valueless_t const&&) noexcept { return true; },
            [](std::in_place_index_t<0>, int const&&) noexcept(true) { return true; },
            [](std::in_place_index_t<1>, double const&&) noexcept(true) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(raw_visit_noexcept<Visitor const&, Storage const&&>);

        constexpr auto const& table = raw_visit_dispatch_table<Visitor const&, Storage const&&>::value;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[variant_npos + 1]), Visitor const&, Storage const&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[0 + 1]), Visitor const&, Storage const&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<true, bool, decltype(table[1 + 1]), Visitor const&, Storage const&&>);
    }
    // invoke(overload{noexcept(true), noexcept(false)}, const&&)
    {
        [[maybe_unused]] auto const vis = yk::overloaded{
            [](std::in_place_index_t<variant_npos>, valueless_t const&&) noexcept { return true; },
            [](std::in_place_index_t<0>, int const&&) noexcept(false) { return true; },
            [](std::in_place_index_t<1>, double const&&) noexcept(false) { return true; },
        };
        using Visitor = decltype(vis);
        STATIC_REQUIRE(!raw_visit_noexcept<Visitor const&, Storage const&&>);

        constexpr auto const& table = raw_visit_dispatch_table<Visitor const&, Storage const&&>::value;
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[variant_npos + 1]), Visitor const&, Storage const&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[0 + 1]), Visitor const&, Storage const&&>);
        STATIC_REQUIRE(is_noexcept_invocable_r_v<false, bool, decltype(table[1 + 1]), Visitor const&, Storage const&&>);
    }

    // NOLINTEND(cppcoreguidelines-rvalue-reference-param-not-moved)
}

namespace {

template<bool... NeverValueless>
using never_valueless_seq = std::integer_sequence<bool, NeverValueless...>;

} // anonymous

TEST_CASE("flat_index", "[detail]")
{
    using yk::detail::flat_index;
    using std::index_sequence;
    {
        STATIC_REQUIRE(flat_index<index_sequence<1>, never_valueless_seq<true>>::get(0) == 0);

        STATIC_REQUIRE(flat_index<index_sequence<2>, never_valueless_seq<true>>::get(0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2>, never_valueless_seq<true>>::get(1) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<1, 1>, never_valueless_seq<true, true>>::get(0, 0) == 0);

        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, never_valueless_seq<true, true>>::get(0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, never_valueless_seq<true, true>>::get(1, 0) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, never_valueless_seq<true, true>>::get(0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, never_valueless_seq<true, true>>::get(0, 1) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<true, true>>::get(0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<true, true>>::get(0, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<true, true>>::get(1, 0) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<true, true>>::get(1, 1) == 3);

        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<true, true>>::get(0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<true, true>>::get(0, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<true, true>>::get(0, 2) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<true, true>>::get(1, 0) == 3);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<true, true>>::get(1, 1) == 4);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<true, true>>::get(1, 2) == 5);

        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 0, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 0, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 0, 2) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 0, 3) == 3);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 1, 0) == 4);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 1, 1) == 5);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 1, 2) == 6);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 1, 3) == 7);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 2, 0) == 8);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 2, 1) == 9);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 2, 2) == 10);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(0, 2, 3) == 11);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 0, 0) == 12);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 0, 1) == 13);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 0, 2) == 14);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 0, 3) == 15);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 1, 0) == 16);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 1, 1) == 17);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 1, 2) == 18);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 1, 3) == 19);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 2, 0) == 20);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 2, 1) == 21);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 2, 2) == 22);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3, 4>, never_valueless_seq<true, true, true>>::get(1, 2, 3) == 23);
    }
    {
        STATIC_REQUIRE(flat_index<index_sequence<1>, never_valueless_seq<false>>::get(-1) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<1>, never_valueless_seq<false>>::get( 0) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<2>, never_valueless_seq<false>>::get(-1) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2>, never_valueless_seq<false>>::get( 0) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2>, never_valueless_seq<false>>::get( 1) == 2);

        STATIC_REQUIRE(flat_index<index_sequence<1, 1>, never_valueless_seq<false, true>>::get(-1, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<1, 1>, never_valueless_seq<false, true>>::get( 0, 0) == 1);

        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, never_valueless_seq<false, true>>::get(-1, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, never_valueless_seq<false, true>>::get( 0, 0) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 1>, never_valueless_seq<false, true>>::get( 1, 0) == 2);

        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, never_valueless_seq<false, true>>::get(-1, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, never_valueless_seq<false, true>>::get(-1, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, never_valueless_seq<false, true>>::get( 0, 0) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<1, 2>, never_valueless_seq<false, true>>::get( 0, 1) == 3);

        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<false, true>>::get(-1,  0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<false, true>>::get(-1,  1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<false, true>>::get( 0,  0) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<false, true>>::get( 0,  1) == 3);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<false, true>>::get( 1,  0) == 4);
        STATIC_REQUIRE(flat_index<index_sequence<2, 2>, never_valueless_seq<false, true>>::get( 1,  1) == 5);

        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get(-1, 0) == 0);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get(-1, 1) == 1);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get(-1, 2) == 2);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get( 0, 0) == 3);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get( 0, 1) == 4);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get( 0, 2) == 5);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get( 1, 0) == 6);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get( 1, 1) == 7);
        STATIC_REQUIRE(flat_index<index_sequence<2, 3>, never_valueless_seq<false, true>>::get( 1, 2) == 8);

        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get(-1, 0, -1) ==  0);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get(-1, 0,  0) ==  1);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get(-1, 0,  1) ==  2);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get(-1, 1, -1) ==  3);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get(-1, 1,  0) ==  4);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get(-1, 1,  1) ==  5);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 0, 0, -1) ==  6);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 0, 0,  0) ==  7);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 0, 0,  1) ==  8);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 0, 1, -1) ==  9);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 0, 1,  0) == 10);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 0, 1,  1) == 11);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 1, 0, -1) == 12);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 1, 0,  0) == 13);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 1, 0,  1) == 14);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 1, 1, -1) == 15);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 1, 1,  0) == 16);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 1, 1,  1) == 17);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 2, 0, -1) == 18);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 2, 0,  0) == 19);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 2, 0,  1) == 20);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 2, 1, -1) == 21);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 2, 1,  0) == 22);
        STATIC_REQUIRE(flat_index<index_sequence<3, 2, 2>, never_valueless_seq<false, true, false>>::get( 2, 1,  1) == 23);
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
R visit(Foo&&, Bar&&...)
{
    return R{"not_a_variant"};
}

template<class Foo, class... Bar>
    requires (!std::disjunction_v<yk::core::is_ttp_specialization_of<std::remove_cvref_t<Bar>, DerivedVariant>...>)
decltype(auto) visit(Foo&&, Bar&&...)
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
    [[maybe_unused]] auto const vis = yk::overloaded{
        [](int const&) -> std::string_view { return "variant"; },
        [](float const&) -> std::string_view { return "variant"; },
    };
    using Visitor = decltype(vis);

    [[maybe_unused]] auto const different_R_vis = yk::overloaded{
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

        static_assert(IntChecker::accepts_all_alternatives);
        static_assert(IntChecker::value);
        static_assert(!DoubleChecker::accepts_all_alternatives);
        static_assert(!DoubleChecker::value);

        {
            using Check = yk::detail::visit_check<std::string_view, Visitor, yk::rvariant<int, float>&&>;
            static_assert(Check::accepts_all_alternatives);
            static_assert(Check::same_return_type);
            static_assert(Check::value);
        }
        {
            using Check = yk::detail::visit_check<std::string_view, DifferentRVisitor, yk::rvariant<int, float>&&>;
            static_assert(Check::accepts_all_alternatives);
            static_assert(!Check::same_return_type);
            static_assert(!Check::value);
        }
        {
            using Check = yk::detail::visit_check<std::string_view, Visitor, yk::rvariant<int, double>&&>;
            static_assert(!Check::accepts_all_alternatives);
            static_assert(!Check::value);
        }
    }
    // for `visit<R>(...)`
    {
        using IntChecker = yk::detail::visit_R_check_impl<std::string_view, Visitor, yk::core::type_list<int>>;
        using DoubleChecker = yk::detail::visit_R_check_impl<std::string_view, Visitor, yk::core::type_list<double>>;

        static_assert(IntChecker::accepts_all_alternatives);
        static_assert(IntChecker::return_type_convertible_to_R);
        static_assert(IntChecker::value);
        static_assert(!DoubleChecker::accepts_all_alternatives);
        static_assert(!DoubleChecker::value);

        {
            using Check = yk::detail::visit_R_check<std::string_view, Visitor, yk::rvariant<int, float>&&>;
            static_assert(Check::accepts_all_alternatives);
            static_assert(Check::return_type_convertible_to_R);
            static_assert(Check::value);
        }
        {
            using Check = yk::detail::visit_R_check<std::string, DifferentRVisitor, yk::rvariant<int, float>&&>;
            static_assert(Check::accepts_all_alternatives);
            static_assert(!Check::return_type_convertible_to_R);
            static_assert(!Check::value);
        }
        {
            using Check = yk::detail::visit_R_check<std::string_view, Visitor, yk::rvariant<int, double>&&>;
            static_assert(!Check::accepts_all_alternatives);
            static_assert(!Check::value);
        }
    }

    {
        // Asserts the "Constraints:" is implemented correctly
        // https://eel.is/c++draft/variant.visit#2
        STATIC_REQUIRE(requires {
            requires std::same_as<std::true_type, SFINAE_context::overload_resolvable<Visitor, not_a_variant_ADL::not_a_variant<int, float>>::type>;
        });

        using std::visit;
        CHECK(visit<std::string_view>(vis, not_a_variant_ADL::not_a_variant<int, float>{}) == "not_a_variant");
        CHECK(visit<std::string_view>(vis, std::variant<int, float>{}) == "variant");
    }
    {
        // Asserts the "Constraints:" is implemented correctly
        // https://eel.is/c++draft/variant.visit#2
        STATIC_REQUIRE(requires {
            requires std::same_as<std::true_type, SFINAE_context::overload_resolvable<Visitor, not_a_variant_ADL::not_a_variant<int, float>>::type>;
        });

        using ::yk::visit;
        CHECK(visit<std::string_view>(vis, not_a_variant_ADL::not_a_variant<int, float>{}) == "not_a_variant");
        CHECK(visit<std::string_view>(vis, yk::rvariant<int, float>{}) == "variant");
        CHECK(visit(vis, not_a_variant_ADL::not_a_variant<int, float>{}) == "not_a_variant");
        CHECK(visit(vis, yk::rvariant<int, float>{}) == "variant");
        CHECK(yk::rvariant<int, float>{}.visit<std::string_view>(vis) == "variant");
        CHECK(yk::rvariant<int, float>{}.visit(vis) == "variant");

        // Asserts as-variant is working
        CHECK(visit<std::string_view>(vis, not_a_variant_ADL::DerivedVariant<int, float>{42}) == "variant");
        CHECK(visit(vis, not_a_variant_ADL::DerivedVariant<int, float>{42}) == "variant");
        CHECK(not_a_variant_ADL::DerivedVariant<int, float>{42}.visit<std::string_view>(vis) == "variant");
        CHECK(not_a_variant_ADL::DerivedVariant<int, float>{42}.visit(vis) == "variant");
    }
    {
        // Not permitted (hard error); std::string_view -> std::string is not implicitly convertible
        //CHECK(std::visit<std::string>(different_R_vis, std::variant<int>{}) == "variant");
        //CHECK(yk::visit<std::string>(different_R_vis, yk::rvariant<int>{}) == "variant");
    }
    {
        // Implicit cast from different types shall be permitted as per INVOKE<R>(...)
        CHECK(std::visit<std::string_view>(different_R_vis, std::variant<int, float>{}) == "variant");
        CHECK(yk::visit<std::string_view>(different_R_vis, yk::rvariant<int, float>{}) == "variant");
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
        auto const vis = yk::overloaded{
            [](SI const&) { return 0; },
            [](SD const&) { return 1; },
        };
        CHECK(std::visit<int>(vis, std::variant<SI, SD>{std::in_place_type<SI>}) == 0);
        CHECK(std::visit<int>(vis, std::variant<SI, SD>{std::in_place_type<SD>}) == 1);

        CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{std::in_place_type<SI>}) == 0);
        CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{std::in_place_type<SD>}) == 1);
        CHECK(yk::visit(vis, yk::rvariant<SI, SD>{std::in_place_type<SI>}) == 0);
        CHECK(yk::visit(vis, yk::rvariant<SI, SD>{std::in_place_type<SD>}) == 1);

        CHECK(yk::rvariant<SI, SD>{std::in_place_type<SI>}.visit<int>(vis) == 0);
        CHECK(yk::rvariant<SI, SD>{std::in_place_type<SD>}.visit<int>(vis) == 1);
        CHECK(yk::rvariant<SI, SD>{std::in_place_type<SI>}.visit(vis) == 0);
        CHECK(yk::rvariant<SI, SD>{std::in_place_type<SD>}.visit(vis) == 1);
    }
    {
        auto const vis = yk::overloaded{
            [](SI const&, SC const&) { return 0; },
            [](SI const&, SW const&) { return 1; },
            [](SD const&, SC const&) { return 2; },
            [](SD const&, SW const&) { return 3; },
        };
        CHECK(std::visit<int>(vis, std::variant<SI, SD>{std::in_place_type<SI>}, std::variant<SC, SW>{std::in_place_type<SC>}) == 0);
        CHECK(std::visit<int>(vis, std::variant<SI, SD>{std::in_place_type<SI>}, std::variant<SC, SW>{std::in_place_type<SW>}) == 1);
        CHECK(std::visit<int>(vis, std::variant<SI, SD>{std::in_place_type<SD>}, std::variant<SC, SW>{std::in_place_type<SC>}) == 2);
        CHECK(std::visit<int>(vis, std::variant<SI, SD>{std::in_place_type<SD>}, std::variant<SC, SW>{std::in_place_type<SW>}) == 3);

        CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{std::in_place_type<SI>}, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 0);
        CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{std::in_place_type<SI>}, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 1);
        CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{std::in_place_type<SD>}, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 2);
        CHECK(yk::visit<int>(vis, yk::rvariant<SI, SD>{std::in_place_type<SD>}, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 3);
        CHECK(yk::visit(vis, yk::rvariant<SI, SD>{std::in_place_type<SI>}, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 0);
        CHECK(yk::visit(vis, yk::rvariant<SI, SD>{std::in_place_type<SI>}, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 1);
        CHECK(yk::visit(vis, yk::rvariant<SI, SD>{std::in_place_type<SD>}, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 2);
        CHECK(yk::visit(vis, yk::rvariant<SI, SD>{std::in_place_type<SD>}, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 3);
    }

    {
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        auto const vis = [](auto&&) {};
        CHECK_THROWS(yk::visit<void>(vis, valueless));
        CHECK_THROWS(yk::visit(vis, valueless));
        CHECK_THROWS(valueless.visit<void>(vis));
        CHECK_THROWS(valueless.visit(vis));
    }
    {
        yk::rvariant<int> never_valueless;
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
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
        auto const vis = yk::overloaded{
            [](SI /* unwrapped */ const&) { return 0; },
            [](SD const&) { return 1; },
        };
        CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SI>}) == 0);
        CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SD>}) == 1);
        CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SI>}) == 0);
        CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SD>}) == 1);

        CHECK(yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SI>}.visit<int>(vis) == 0);
        CHECK(yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SD>}.visit<int>(vis) == 1);
        CHECK(yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SI>}.visit(vis) == 0);
        CHECK(yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SD>}.visit(vis) == 1);
    }
    {
        auto const vis = yk::overloaded{
            [](SI /* unwrapped */ const&, SC const&) { return 0; },
            [](SI /* unwrapped */ const&, SW const&) { return 1; },
            [](SD const&, SC const&) { return 2; },
            [](SD const&, SW const&) { return 3; },
        };
        CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SI>}, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 0);
        CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SI>}, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 1);
        CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SD>}, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 2);
        CHECK(yk::visit<int>(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SD>}, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 3);
        CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SI>}, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 0);
        CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SI>}, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 1);
        CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SD>}, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 2);
        CHECK(yk::visit(vis, yk::rvariant<yk::recursive_wrapper<SI>, SD>{std::in_place_type<SD>}, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 3);
    }
}

}
