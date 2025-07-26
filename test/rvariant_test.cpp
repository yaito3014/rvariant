#include "rvariant_test.hpp"

#include "yk/rvariant/recursive_wrapper.hpp"
#include "yk/rvariant/rvariant.hpp"
#include "yk/rvariant/pack.hpp"

#include "yk/indirect.hpp"

#include <catch2/catch_test_macros.hpp>

#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <algorithm>
#include <ranges>
#include <bit>
#include <array>

#include <cstddef>

namespace unit_test {

TEST_CASE("make_valueless", "[detail]")
{
    yk::rvariant<int, MoveThrows> valueless = make_valueless<int>(42);
    CHECK(valueless.valueless_by_exception());
    CHECK(valueless.index() == std::variant_npos);
    STATIC_CHECK(!std::remove_cvref_t<decltype(valueless)>::never_valueless);
}

TEST_CASE("storage", "[detail]")
{
    // Test cases for this type is VERY important; some compilers misunderstand relevant type traits
    struct NonExistent {};

    // NOLINTBEGIN(modernize-use-equals-default)
    {
        using T = int;
        using VD = yk::detail::variadic_union<true, T>;
        STATIC_REQUIRE(std::is_same_v<yk::detail::make_variadic_union_t<T>, VD>);
        {
            STATIC_REQUIRE(std::is_nothrow_default_constructible_v<VD>);   // valueless

            STATIC_REQUIRE(std::is_trivially_copy_constructible_v<VD>);
            STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<VD>);

            STATIC_REQUIRE(std::is_trivially_move_constructible_v<VD>);
            STATIC_REQUIRE(std::is_nothrow_move_constructible_v<VD>);

            STATIC_REQUIRE(std::is_trivially_copy_assignable_v<VD>);
            STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<VD>);

            STATIC_REQUIRE(std::is_trivially_move_assignable_v<VD>);
            STATIC_REQUIRE(std::is_nothrow_move_assignable_v<VD>);

            STATIC_REQUIRE(std::is_trivially_destructible_v<VD>);
            STATIC_REQUIRE(std::is_nothrow_destructible_v<VD>);

            STATIC_REQUIRE(std::is_trivially_copyable_v<VD>);

            STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, std::in_place_index_t<0>>); // default construct
            STATIC_REQUIRE(std::is_constructible_v<VD, std::in_place_index_t<0>, T>);
            STATIC_REQUIRE(!std::is_constructible_v<VD, std::in_place_index_t<0>, NonExistent>);
        }

        using V = yk::rvariant<T>;

        using Base = yk::detail::rvariant_base<T>;
        {
            STATIC_REQUIRE(std::is_trivially_copy_constructible_v<Base>);
            STATIC_REQUIRE(std::is_trivially_move_constructible_v<Base>);
            STATIC_REQUIRE(std::is_trivially_copy_assignable_v<Base>);
            STATIC_REQUIRE(std::is_trivially_move_assignable_v<Base>);
            STATIC_REQUIRE(std::is_trivially_destructible_v<Base>);
            STATIC_REQUIRE(std::is_trivially_copyable_v<Base>);
        }
        {
            STATIC_REQUIRE(std::is_trivially_copy_constructible_v<V>);
            STATIC_REQUIRE(std::is_trivially_move_constructible_v<V>);
            STATIC_REQUIRE(std::is_trivially_copy_assignable_v<V>);
            STATIC_REQUIRE(std::is_trivially_move_assignable_v<V>);
            STATIC_REQUIRE(std::is_trivially_destructible_v<V>);
            STATIC_REQUIRE(std::is_trivially_copyable_v<V>);
        }
    }

    {
        struct S
        {
            S() = default;
            S(S const&) noexcept {} // non-trivial
            S(S&&) = default;
            ~S() = default;
            S& operator=(S const&) = default;
            S& operator=(S&&) = default;
        };
        using T = S;
        {
            STATIC_REQUIRE(!std::is_trivially_copy_constructible_v<T>);
            STATIC_REQUIRE(std::is_trivially_move_constructible_v<T>);
            STATIC_REQUIRE(std::is_trivially_copy_assignable_v<T>);
            STATIC_REQUIRE(std::is_trivially_move_assignable_v<T>);
            STATIC_REQUIRE(std::is_trivially_destructible_v<T>);
            STATIC_REQUIRE(!std::is_trivially_copyable_v<T>);
        }

        using V = yk::rvariant<T>;

        using Base = yk::detail::rvariant_base_t<T>;
        static_assert(std::is_base_of_v<Base, V>);
        {
            STATIC_REQUIRE(!std::is_trivially_copy_constructible_v<Base>);
            STATIC_REQUIRE(std::is_trivially_move_constructible_v<Base>);
            STATIC_REQUIRE(!std::is_trivially_copy_assignable_v<Base>); // variant requires TCC && TCA && TD
            STATIC_REQUIRE(std::is_trivially_move_assignable_v<Base>);
            STATIC_REQUIRE(std::is_trivially_destructible_v<Base>);
            STATIC_REQUIRE(!std::is_trivially_copyable_v<Base>);
        }
        {
            STATIC_REQUIRE(!std::is_trivially_copy_constructible_v<V>);
            STATIC_REQUIRE(std::is_trivially_move_constructible_v<V>);
            STATIC_REQUIRE(!std::is_trivially_copy_assignable_v<V>); // variant requires TCC && TCA && TD
            STATIC_REQUIRE(std::is_trivially_move_assignable_v<V>);
            STATIC_REQUIRE(std::is_trivially_destructible_v<V>);
            STATIC_REQUIRE(!std::is_trivially_copyable_v<V>);
        }
    }

    // non-trivial
    {
        struct S
        {
            S() = default;
            S(S const&) {} // non-trivial
            S(S&&) = default;
            ~S() = default;
            S& operator=(S const&) = default;
            S& operator=(S&&) = default;
        };
        STATIC_REQUIRE(!std::is_trivially_copy_constructible_v<S>);
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<S>);
        STATIC_REQUIRE(std::is_trivially_copy_assignable_v<S>);
        STATIC_REQUIRE(std::is_trivially_move_assignable_v<S>);
        STATIC_REQUIRE(std::is_trivially_destructible_v<S>);
        STATIC_REQUIRE(!std::is_trivially_copyable_v<S>);

        using V = yk::rvariant<S>;
        STATIC_REQUIRE( std::is_copy_constructible_v<V>);
        STATIC_REQUIRE(!std::is_trivially_copy_constructible_v<V>);

        STATIC_REQUIRE(std::is_move_constructible_v<V>);
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<V>);

        //STATIC_REQUIRE(std::is_trivially_copy_assignable_v<V>);
        STATIC_REQUIRE(std::is_trivially_move_assignable_v<V>);
        STATIC_REQUIRE(std::is_trivially_destructible_v<V>);
        STATIC_REQUIRE(!std::is_trivially_copyable_v<V>);
    }

    {
        struct S
        {
            S() = default;
            S(int) {}
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

        STATIC_REQUIRE(std::is_trivially_destructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_destructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copyable_v<VD>);

        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, std::in_place_index_t<0>>); // default construct
        STATIC_REQUIRE( std::is_constructible_v<VD, std::in_place_index_t<0>, S>);
        STATIC_REQUIRE( std::is_constructible_v<VD, std::in_place_index_t<0>, int>);
        STATIC_REQUIRE(!std::is_constructible_v<VD, std::in_place_index_t<0>, NonExistent>);
    }
    // `int` on the right side
    {
        struct S
        {
            S() = default;
            S(int) {}
        };
        using VD = yk::detail::variadic_union<true, S, int>;

        STATIC_REQUIRE(std::is_nothrow_default_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_move_constructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_move_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copy_assignable_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<VD>);

        STATIC_REQUIRE(std::is_trivially_move_assignable_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_move_assignable_v<VD>);

        STATIC_REQUIRE(std::is_trivially_destructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_destructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copyable_v<VD>);

        // for VD[0] aka `S`
        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, std::in_place_index_t<0>>); // default construct
        STATIC_REQUIRE( std::is_constructible_v<VD, std::in_place_index_t<0>, S>);
        STATIC_REQUIRE( std::is_constructible_v<VD, std::in_place_index_t<0>, int>);
        STATIC_REQUIRE(!std::is_constructible_v<VD, std::in_place_index_t<0>, NonExistent>);

        // for VD[1] aka `int`
        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, std::in_place_index_t<1>>); // default construct
        STATIC_REQUIRE( std::is_constructible_v<VD, std::in_place_index_t<1>, int>);
        STATIC_REQUIRE(!std::is_constructible_v<VD, std::in_place_index_t<1>, S>);
        STATIC_REQUIRE(!std::is_constructible_v<VD, std::in_place_index_t<1>, NonExistent>);
    }
    // `int` on the left side
    {
        struct S
        {
            S() = default;
            S(int) {}
        };
        using VD = yk::detail::variadic_union<true, int, S>;

        STATIC_REQUIRE(std::is_nothrow_default_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copy_constructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_move_constructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_move_constructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copy_assignable_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<VD>);

        STATIC_REQUIRE(std::is_trivially_move_assignable_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_move_assignable_v<VD>);

        STATIC_REQUIRE(std::is_trivially_destructible_v<VD>);
        STATIC_REQUIRE(std::is_nothrow_destructible_v<VD>);

        STATIC_REQUIRE(std::is_trivially_copyable_v<VD>);

        // for VD[0] aka `int`
        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, std::in_place_index_t<0>>); // default construct
        STATIC_REQUIRE( std::is_constructible_v<VD, std::in_place_index_t<0>, int>);
        STATIC_REQUIRE(!std::is_constructible_v<VD, std::in_place_index_t<0>, S>);
        STATIC_REQUIRE(!std::is_constructible_v<VD, std::in_place_index_t<0>, NonExistent>);

        // for VD[1] aka `S`
        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, std::in_place_index_t<1>>); // default construct
        STATIC_REQUIRE( std::is_constructible_v<VD, std::in_place_index_t<1>, S>);
        STATIC_REQUIRE( std::is_constructible_v<VD, std::in_place_index_t<1>, int>);
        STATIC_REQUIRE(!std::is_constructible_v<VD, std::in_place_index_t<1>, NonExistent>);
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

        STATIC_REQUIRE(!std::is_trivially_destructible_v<VD>);
        STATIC_REQUIRE(!std::is_trivially_copyable_v<VD>);

        STATIC_REQUIRE(std::is_nothrow_constructible_v<VD, std::in_place_index_t<0>>); // default construct
        STATIC_REQUIRE(std::is_nothrow_destructible_v<VD>);
    }
    // NOLINTEND(modernize-use-equals-default)
}

namespace {

template<class Variant>
[[nodiscard]] int test_forward_storage(Variant&& v)  // NOLINT(cppcoreguidelines-missing-std-forward)
{
    using yk::detail::forward_storage_t;
    using yk::detail::forward_storage;
    using yk::detail::as_variant_t;
    using V = std::remove_cvref_t<Variant>;
    using ExactV = yk::rvariant<int>;
    using VU = yk::detail::make_variadic_union_t<int>;

    constexpr bool IsExact = yk::core::is_ttp_specialization_of_v<V, yk::rvariant>;

    if constexpr (std::is_same_v<Variant&&, V&>) {
        STATIC_REQUIRE(std::is_same_v<as_variant_t<Variant>, ExactV&>);
        if constexpr (IsExact) {
            STATIC_REQUIRE(std::is_same_v<forward_storage_t<Variant>, VU&>);
            STATIC_REQUIRE(std::is_same_v<decltype(forward_storage<Variant>(v)), VU&>);
        }
        STATIC_REQUIRE(std::is_same_v<forward_storage_t<as_variant_t<Variant>>, VU&>);
        STATIC_REQUIRE(std::is_same_v<decltype(forward_storage<as_variant_t<Variant>>(v)), VU&>);
        return 0;

    } else if constexpr (std::is_same_v<Variant&&, V const&>) {
        STATIC_REQUIRE(std::is_same_v<as_variant_t<Variant>, ExactV const&>);
        if constexpr (IsExact) {
            STATIC_REQUIRE(std::is_same_v<forward_storage_t<Variant>, VU const&>);
            STATIC_REQUIRE(std::is_same_v<decltype(forward_storage<Variant>(v)), VU const&>);
        }
        STATIC_REQUIRE(std::is_same_v<forward_storage_t<as_variant_t<Variant>>, VU const&>);
        STATIC_REQUIRE(std::is_same_v<decltype(forward_storage<as_variant_t<Variant>>(v)), VU const&>);
        return 1;

    } else if constexpr (std::is_same_v<Variant&&, V&&>) {
        STATIC_REQUIRE(std::is_same_v<as_variant_t<Variant>, ExactV&&>);
        if constexpr (IsExact) {
            STATIC_REQUIRE(std::is_same_v<forward_storage_t<Variant>, VU&&>);
            STATIC_REQUIRE(std::is_same_v<decltype(forward_storage<Variant>(v)), VU&&>);
        }
        STATIC_REQUIRE(std::is_same_v<forward_storage_t<as_variant_t<Variant>>, VU&&>);
        STATIC_REQUIRE(std::is_same_v<decltype(forward_storage<as_variant_t<Variant>>(v)), VU&&>);
        return 2;

    } else if constexpr (std::is_same_v<Variant&&, V const&&>) {
        STATIC_REQUIRE(std::is_same_v<as_variant_t<Variant>, ExactV const&&>);
        if constexpr (IsExact) {
            STATIC_REQUIRE(std::is_same_v<forward_storage_t<Variant>, VU const&&>);
            STATIC_REQUIRE(std::is_same_v<decltype(forward_storage<Variant>(v)), VU const&&>);
        }
        STATIC_REQUIRE(std::is_same_v<forward_storage_t<as_variant_t<Variant>>, VU const&&>);
        STATIC_REQUIRE(std::is_same_v<decltype(forward_storage<as_variant_t<Variant>>(v)), VU const&&>);
        return 3;

    } else {
        static_assert(false);
        return -1;
    }
}

template<class... Ts>
struct DerivedVariant : yk::rvariant<Ts...>
{
    using DerivedVariant::rvariant::rvariant;
};

} // anonymous

TEST_CASE("forward_storage", "[detail]")
{
    // NOLINTBEGIN(performance-move-const-arg)
    {
        yk::rvariant<int> v;
        CHECK(test_forward_storage(v) == 0);
        CHECK(test_forward_storage(std::as_const(v)) == 1);
        CHECK(test_forward_storage(std::move(std::as_const(v))) == 3);
        CHECK(test_forward_storage(std::move(v)) == 2);
    }
    {
        DerivedVariant<int> v;
        CHECK(test_forward_storage(v) == 0);
        CHECK(test_forward_storage(std::as_const(v)) == 1);
        CHECK(test_forward_storage(std::move(std::as_const(v))) == 3);
        CHECK(test_forward_storage(std::move(v)) == 2);
    }
    // NOLINTEND(performance-move-const-arg)
}

TEST_CASE("default construction")
{
    {
        struct S
        {
            S() = delete;
        };

        STATIC_REQUIRE(!std::is_default_constructible_v<yk::rvariant<S>>);
        STATIC_REQUIRE(!std::is_default_constructible_v<yk::rvariant<S, int>>);
        STATIC_REQUIRE( std::is_default_constructible_v<yk::rvariant<int, S>>);
    }

    // value-initialize
    {
        STATIC_CHECK(std::is_trivially_copy_constructible_v<yk::rvariant<int>>);
        STATIC_CHECK(std::is_trivially_move_constructible_v<yk::rvariant<int>>);
        STATIC_CHECK(std::is_trivially_copy_assignable_v<yk::rvariant<int>>);
        STATIC_CHECK(std::is_trivially_move_assignable_v<yk::rvariant<int>>);
        STATIC_CHECK(std::is_trivially_destructible_v<yk::rvariant<int>>);
        STATIC_CHECK(std::is_trivially_copyable_v<yk::rvariant<int>>);

        // try to test in constexpr context to detect UB
        constexpr auto default_constructed_value = []() constexpr -> int {
            using V = yk::rvariant<int>;
            alignas(V) std::array<std::byte, sizeof(V)> storage;
            {
                V arbitrary_value(42);
                storage = std::bit_cast<decltype(storage)>(arbitrary_value);
            }
            V* v_ptr = new (&storage) V; // default-initialize
            int value = yk::get<int>(*v_ptr); // MUST be value-initialized as per https://eel.is/c++draft/variant.ctor#3
            v_ptr->~V();
            return value;
        };

#if __cpp_lib_constexpr_new >= 202406L
        STATIC_CHECK(default_constructed_value() == 0);
#else
        CHECK(default_constructed_value() == 0);
#endif
    }

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

    {
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        yk::rvariant<int, MoveThrows> a(valueless);
        CHECK(a.valueless_by_exception());
    }
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
        static_assert(!std::is_move_assignable_v<S>);
        yk::rvariant<S> a;
        static_assert(std::is_move_constructible_v<yk::rvariant<S>>);
        REQUIRE_THROWS(yk::rvariant<S>(std::move(a)));
    }

    {
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        yk::rvariant<int, MoveThrows> a(std::move(valueless));
        CHECK(a.valueless_by_exception());
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

TEST_CASE("in_place_index construction")
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

TEST_CASE("in_place_type construction")
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


TEST_CASE("copy assignment")
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

    {
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        yk::rvariant<int, MoveThrows> a;
        a = valueless;
        CHECK(a.valueless_by_exception());
    }

    // NOLINTEND(modernize-use-equals-default)
}

TEST_CASE("move assignment")
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

    {
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        yk::rvariant<int, MoveThrows> a;
        a = std::move(valueless);
        CHECK(a.valueless_by_exception());
    }
}

TEST_CASE("generic assignment")
{
    {
        yk::rvariant<int, float> a{42};
        CHECK(a.index() == 0);

        a = 33;
        CHECK(a.index() == 0);

        a = 3.14f;
        CHECK(a.index() == 1);
    }

    {
        yk::rvariant<int, MoveThrows> a;
        try {
            a = MoveThrows::non_throwing;
        } catch(...) {
        }
        CHECK(!a.valueless_by_exception());
    }
    {
        yk::rvariant<int, MoveThrows> a;
        try {
            a = MoveThrows::throwing;
        } catch(...) {
        }
        CHECK(!a.valueless_by_exception());
    }
    {
        yk::rvariant<int, MoveThrows> a;
        try {
            a = MoveThrows::potentially_throwing;
        } catch(...) {
        }
        CHECK(a.valueless_by_exception());
    }
}

TEST_CASE("emplace")
{
    {
        yk::rvariant<int> a;
        STATIC_REQUIRE(std::is_same_v<decltype(a.emplace<int>()), int&>);
        STATIC_REQUIRE(std::is_same_v<decltype(std::move(a).emplace<int>()), int&>);
    }

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

    {
        yk::rvariant<MoveThrows> a;
        try {
            a.emplace<0>(MoveThrows::non_throwing);
        } catch(...) {
        }
        CHECK(!a.valueless_by_exception());
    }
    {
        yk::rvariant<MoveThrows> a;
        try {
            a.emplace<0>(MoveThrows::throwing);
        } catch(...) {
        }
        CHECK(!a.valueless_by_exception());
    }
    {
        yk::rvariant<MoveThrows> a;
        try {
            a.emplace<0>(MoveThrows::potentially_throwing);
        } catch(...) {
        }
        CHECK(a.valueless_by_exception());
    }
}

namespace {

template<std::size_t I>
struct just_index {};

} // anonymous

TEST_CASE("swap")
{
    {
        STATIC_REQUIRE(yk::core::is_trivially_swappable_v<int>);
        yk::rvariant<int> a = 33, b = 4;
        REQUIRE_NOTHROW(a.swap(b));
        CHECK(yk::get<0>(a) == 4);
        CHECK(yk::get<0>(b) == 33);
    }
    {
        yk::rvariant<int, float> a = 42, b = 3.14f;
        REQUIRE_NOTHROW(a.swap(b));
        CHECK(yk::get<1>(a) == 3.14f);
        CHECK(yk::get<0>(b) == 42);
    }
    {
        struct S
        {
            S() = default;
            S(S const&) = default;
            S(S&&) {}
            S& operator=(S const&) = default;
            S& operator=(S&&) { return *this; }
        };

        STATIC_REQUIRE(!yk::core::is_trivially_swappable_v<S>);
        yk::rvariant<int, S> a{42}, b{S{}};
        REQUIRE_NOTHROW(a.swap(b));
        CHECK(yk::holds_alternative<S>(a));
        CHECK(yk::holds_alternative<int>(b));
    }
    {
        using V = yk::rvariant<
            just_index<0>, just_index<1>, just_index<2>, just_index<3>, just_index<4>, just_index<5>, just_index<6>, just_index<7>,
            just_index<8>, just_index<9>, just_index<10>, just_index<11>, just_index<12>, just_index<13>, just_index<14>, just_index<15>,
            just_index<16>, just_index<17>, just_index<18>, just_index<19>, just_index<20>, just_index<21>, just_index<22>, just_index<23>,
            just_index<24>, just_index<25>, just_index<26>, just_index<27>, just_index<28>, just_index<29>, just_index<30>, just_index<31>
        >;
        static_assert((yk::variant_size_v<V> * yk::variant_size_v<V>) >= yk::detail::visit_instantiation_limit);

        V a{std::in_place_type<just_index<10>>}, b{std::in_place_type<just_index<20>>};
        REQUIRE_NOTHROW(a.swap(b));
        CHECK(yk::holds_alternative<just_index<20>>(a));
        CHECK(yk::holds_alternative<just_index<10>>(b));
    }

    {
        yk::rvariant<int, MoveThrows> a = make_valueless<int>();
        yk::rvariant<int, MoveThrows> b;
        CHECK( a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
        try {
            a.swap(b);
        } catch (...) {
        }
        CHECK(!a.valueless_by_exception());
        CHECK( b.valueless_by_exception());
    }
    {
        yk::rvariant<int, MoveThrows> a;
        yk::rvariant<int, MoveThrows> b = make_valueless<int>();
        CHECK(!a.valueless_by_exception());
        CHECK( b.valueless_by_exception());
        try {
            a.swap(b);
        } catch (...) {
        }
        CHECK( a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
    }
    {
        yk::rvariant<int, MoveThrows> a = make_valueless<int>();
        yk::rvariant<int, MoveThrows> b = make_valueless<int>();
        CHECK(a.valueless_by_exception());
        CHECK(b.valueless_by_exception());
        CHECK_NOTHROW(a.swap(b));
        CHECK(a.valueless_by_exception());
        CHECK(b.valueless_by_exception());
    }
    {
        yk::rvariant<int, MoveThrows> a = make_valueless<int>();
        yk::rvariant<int, MoveThrows> b(std::in_place_type<MoveThrows>);
        CHECK( a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
        try {
            a.swap(b);
        } catch (...) {
        }
        CHECK( a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
    }
    {
        yk::rvariant<int, MoveThrows> a(std::in_place_type<int>);
        yk::rvariant<int, MoveThrows> b(std::in_place_type<MoveThrows>);
        CHECK(!a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
        try {
            a.swap(b);
        } catch (...) {
        }
        CHECK( a.valueless_by_exception());  // swap-ee          becomes valueless
        CHECK(!b.valueless_by_exception());  // swap-er does not become  valueless
    }
}

TEST_CASE("holds_alternative")
{
    {
        yk::rvariant<int, float> var = 42;
        CHECK(yk::holds_alternative<int>(var));
    }

    {
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        CHECK(!yk::holds_alternative<int>(valueless));
        CHECK(!yk::holds_alternative<MoveThrows>(valueless));
    }
}

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4244) // implicit numeric conversion
#endif

TEST_CASE("recursive_wrapper") // not [recursive]
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

#ifdef _MSC_VER
# pragma warning(pop)
#endif

TEST_CASE("unwrap_recursive") // not [recursive]
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

TEST_CASE("maybe_wrapped") // not [recursive]
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

TEST_CASE("non_recursive_same_as_std") // not [recursive]
{
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

TEST_CASE("relational operators")
{
    {
        yk::rvariant<int> a = 33, b = 4;
        CHECK(a == a);
        CHECK(!(a == b));

        CHECK(a != b);
        CHECK(!(a != a));

        CHECK(b < a);
        CHECK(!(a < a));
        CHECK(!(a < b));

        CHECK(a > b);
        CHECK(!(a > a));
        CHECK(!(b > a));

        CHECK(b <= a);
        CHECK(a <= a);
        CHECK(!(a <= b));

        CHECK(a >= b);
        CHECK(a >= a);
        CHECK(!(b >= a));

        CHECK((a <=> a) == std::strong_ordering::equal);
        CHECK((b <=> a) == std::strong_ordering::less);
        CHECK((a <=> b) == std::strong_ordering::greater);
    }
    {
        yk::rvariant<int, double> a = 42, b = 3.14;

        CHECK(a == a);
        CHECK(!(a == b));

        CHECK(a != b);
        CHECK(!(a != a));

        CHECK(a < b);
        CHECK(!(b < a));

        CHECK(b > a);
        CHECK(!(a > b));

        CHECK(a <= b);
        CHECK(!(b <= a));

        CHECK(b >= a);
        CHECK(!(a >= b));

        CHECK((a <=> a) == std::partial_ordering::equivalent);
        CHECK((a <=> b) == std::partial_ordering::less);
        CHECK((b <=> a) == std::partial_ordering::greater);
    }
    {
        yk::rvariant<int, MoveThrows> valueless = make_valueless<int>();
        yk::rvariant<int, MoveThrows> a;

        CHECK(valueless == valueless);
        CHECK(!(a == valueless));

        CHECK(!(valueless != valueless));

        CHECK(valueless < a);
        CHECK(!(a < valueless));

        CHECK(a > valueless);
        CHECK(!(valueless > a));

        CHECK(valueless <= a);
        CHECK(!(a <= valueless));

        CHECK(a >= valueless);
        CHECK(!(valueless >= a));

        CHECK((valueless <=> valueless) == std::strong_ordering::equal);
        CHECK((valueless <=> a) == std::strong_ordering::less);
        CHECK((a <=> valueless) == std::strong_ordering::greater);
    }
    {
        std::vector<yk::rvariant<int, double>> vec {
            42,
            33,
            4,
            3.141,
            2.718,
            1.414,
            1.618,
        };
        std::ranges::sort(vec);
        CHECK(vec == std::vector<yk::rvariant<int, double>>{
            4,
            33,
            42,
            1.414,
            1.618,
            2.718,
            3.141,
        });
    }
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

TEST_CASE("pack_union", "[pack][detail]")
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

TEST_CASE("compact_alternative", "[pack]")
{
    STATIC_REQUIRE(std::is_same_v<yk::compact_alternative_t<yk::rvariant, int, float>, yk::rvariant<int, float>>);
    STATIC_REQUIRE(std::is_same_v<yk::compact_alternative_t<yk::rvariant, int, int>, int>);
}

}  // unit_test
