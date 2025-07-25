#include "yk/rvariant/rvariant.hpp"
#include "yk/rvariant/recursive_wrapper.hpp"
#include "yk/rvariant/variant_helper.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <string>
#include <type_traits>
#include <utility>

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

namespace detail {

template<bool AllNeverValueless, class Seq, auto... n>
struct never_valueless_seq_impl;

template<bool AllNeverValueless, class Seq>
struct never_valueless_seq_impl<AllNeverValueless, Seq>
{
    using type = Seq;
};

template<bool AllNeverValueless, auto... n, auto First, auto... Rest>
struct never_valueless_seq_impl<AllNeverValueless, std::integer_sequence<bool, n...>, First, Rest...>
    : never_valueless_seq_impl<AllNeverValueless, std::integer_sequence<bool, n..., AllNeverValueless>, Rest...>
{};

} // detail

// generate (bool, bool, ...) for testing
template<bool AllNeverValueless, auto... n>
using never_valueless_seq = typename detail::never_valueless_seq_impl<AllNeverValueless, std::integer_sequence<bool>, n...>::type;

template<auto... n>
using nv_true_flat_index = yk::detail::flat_index<std::index_sequence<n...>, never_valueless_seq<true, n...>>;

} // anonymous

TEST_CASE("flat_index", "[detail]")
{
    static constexpr bool NeverValueless = true;

    constexpr auto bias = [](std::size_t i) {
        return yk::detail::valueless_bias<NeverValueless>(i);
    };

    STATIC_REQUIRE(nv_true_flat_index<1>::get(0) == bias(0));

    STATIC_REQUIRE(nv_true_flat_index<2>::get(0) == bias(0));
    STATIC_REQUIRE(nv_true_flat_index<2>::get(1) == bias(1));

    STATIC_REQUIRE(nv_true_flat_index<1, 1>::get(0, 0) == bias(0));

    STATIC_REQUIRE(nv_true_flat_index<2, 1>::get(0, 0) == bias(0));
    STATIC_REQUIRE(nv_true_flat_index<2, 1>::get(1, 0) == bias(1));

    STATIC_REQUIRE(nv_true_flat_index<1, 2>::get(0, 0) == bias(0));
    STATIC_REQUIRE(nv_true_flat_index<1, 2>::get(0, 1) == bias(1));

    STATIC_REQUIRE(nv_true_flat_index<2, 2>::get(0, 0) == bias(0));
    STATIC_REQUIRE(nv_true_flat_index<2, 2>::get(0, 1) == bias(1));
    STATIC_REQUIRE(nv_true_flat_index<2, 2>::get(1, 0) == bias(2));
    STATIC_REQUIRE(nv_true_flat_index<2, 2>::get(1, 1) == bias(3));

    STATIC_REQUIRE(nv_true_flat_index<2, 3>::get(0, 0) == bias(0));
    STATIC_REQUIRE(nv_true_flat_index<2, 3>::get(0, 1) == bias(1));
    STATIC_REQUIRE(nv_true_flat_index<2, 3>::get(0, 2) == bias(2));
    STATIC_REQUIRE(nv_true_flat_index<2, 3>::get(1, 0) == bias(3));
    STATIC_REQUIRE(nv_true_flat_index<2, 3>::get(1, 1) == bias(4));
    STATIC_REQUIRE(nv_true_flat_index<2, 3>::get(1, 2) == bias(5));

    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 0, 0) == bias(0));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 0, 1) == bias(1));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 0, 2) == bias(2));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 0, 3) == bias(3));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 1, 0) == bias(4));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 1, 1) == bias(5));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 1, 2) == bias(6));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 1, 3) == bias(7));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 2, 0) == bias(8));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 2, 1) == bias(9));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 2, 2) == bias(10));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(0, 2, 3) == bias(11));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 0, 0) == bias(12));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 0, 1) == bias(13));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 0, 2) == bias(14));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 0, 3) == bias(15));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 1, 0) == bias(16));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 1, 1) == bias(17));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 1, 2) == bias(18));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 1, 3) == bias(19));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 2, 0) == bias(20));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 2, 1) == bias(21));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 2, 2) == bias(22));
    STATIC_REQUIRE(nv_true_flat_index<2, 3, 4>::get(1, 2, 3) == bias(23));
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

} // not_a_variant_ADL

namespace SFINAE_context {

using ::yk::visit;
using ::not_a_variant_ADL::visit;

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

        // Asserts as-variant is working
        CHECK(visit<std::string_view>(vis, not_a_variant_ADL::DerivedVariant<int, float>{42}) == "variant");
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
    }
    {
        auto const vis = yk::overloaded{
            [](SC const&) { return 0; },
            [](SW const&) { return 1; },
        };
        CHECK(std::visit<int>(vis, std::variant<SC, SW>{std::in_place_type<SC>}) == 0);
        CHECK(std::visit<int>(vis, std::variant<SC, SW>{std::in_place_type<SW>}) == 1);

        CHECK(yk::visit<int>(vis, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 0);
        CHECK(yk::visit<int>(vis, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 1);
        CHECK(yk::visit(vis, yk::rvariant<SC, SW>{std::in_place_type<SC>}) == 0);
        CHECK(yk::visit(vis, yk::rvariant<SC, SW>{std::in_place_type<SW>}) == 1);
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

    // TODO: valueless case
}
