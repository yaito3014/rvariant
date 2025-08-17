// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "rvariant_test.hpp"

#include "yk/core/type_traits.hpp"
#include "yk/core/hash.hpp"

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
#include <print>

#include <cstddef>

namespace unit_test {

template<class Variant>
constexpr bool is_never_valueless = ::yk::detail::valueless_bias<Variant>(0) == 0;

TEST_CASE("make_valueless", "[detail]")
{
    yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>(42);
    CHECK(valueless.valueless_by_exception());
    CHECK(valueless.index() == std::variant_npos);
    // ReSharper disable once CppStaticAssertFailure
    STATIC_CHECK(!is_never_valueless<decltype(valueless)>);
}

TEST_CASE("never_valueless", "[detail]")
{
    struct BadType
    {
        BadType(BadType&&) noexcept {}  // NOLINT(modernize-use-equals-default)
        BadType(BadType const&) noexcept {}  // NOLINT(modernize-use-equals-default)

        BadType() = default;
        BadType& operator=(BadType const&) = default;
        BadType& operator=(BadType&&) = default;
    };
    STATIC_REQUIRE(!yk::detail::is_never_valueless_v<BadType>);
    STATIC_REQUIRE(!is_never_valueless<yk::rvariant<BadType>>);

    // Test many minimal trait combinations for variant to be never_valueless.
    {
        struct S
        {
            S(S const&) = delete;
            S& operator=(S const&) = delete;

            S(S&&) = default;
            S& operator=(S&&) = default;
        };
        static_assert(!std::is_trivially_copy_constructible_v<S>);
        static_assert(!std::is_trivially_copy_assignable_v<S>);
        static_assert( std::is_trivially_move_constructible_v<S>);
        static_assert( std::is_trivially_move_assignable_v<S>);
        static_assert( std::is_standard_layout_v<S>);

        STATIC_REQUIRE(yk::detail::is_never_valueless_v<S>);
        STATIC_REQUIRE(is_never_valueless<yk::rvariant<S>>);
        STATIC_REQUIRE( std::is_standard_layout_v<yk::rvariant<S>>);
        STATIC_REQUIRE(!is_never_valueless<yk::rvariant<S, BadType>>);
    }
    {
        // strange type, but...
        struct S
        {
            S(S const&) = default;
            S& operator=(S const&) = default;

            S(S&&) = delete;
            S& operator=(S&&) = delete;
        };
        static_assert( std::is_trivially_copy_constructible_v<S>);
        static_assert( std::is_trivially_copy_assignable_v<S>);
        static_assert(!std::is_trivially_move_constructible_v<S>);
        static_assert(!std::is_trivially_move_assignable_v<S>);

        STATIC_REQUIRE(yk::detail::is_never_valueless_v<S>);
        STATIC_REQUIRE(is_never_valueless<yk::rvariant<S>>);
        STATIC_REQUIRE(!is_never_valueless<yk::rvariant<S, BadType>>);
    }
    {
        // strange type, but...
        struct S
        {
            S(S const&) = delete;
            S& operator=(S const&) = default;

            S(S&&) = default;
            S& operator=(S&&) = delete;
        };
        static_assert(!std::is_trivially_copy_constructible_v<S>);
        static_assert( std::is_trivially_copy_assignable_v<S>);
        static_assert( std::is_trivially_move_constructible_v<S>);
        static_assert(!std::is_trivially_move_assignable_v<S>);

        STATIC_REQUIRE(yk::detail::is_never_valueless_v<S>);
        STATIC_REQUIRE(is_never_valueless<yk::rvariant<S>>);
        STATIC_REQUIRE(!yk::detail::is_never_valueless_v<yk::rvariant<S>>); // wow https://eel.is/c++draft/variant.assign#5
        STATIC_REQUIRE(!is_never_valueless<yk::rvariant<S, BadType>>);
    }
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
            STATIC_REQUIRE(std::is_standard_layout_v<VD>);

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
            STATIC_REQUIRE(std::is_standard_layout_v<Base>);
        }
        {
            STATIC_REQUIRE(std::is_trivially_copy_constructible_v<V>);
            STATIC_REQUIRE(std::is_trivially_move_constructible_v<V>);
            STATIC_REQUIRE(std::is_trivially_copy_assignable_v<V>);
            STATIC_REQUIRE(std::is_trivially_move_assignable_v<V>);
            STATIC_REQUIRE(std::is_trivially_destructible_v<V>);
            STATIC_REQUIRE(std::is_trivially_copyable_v<V>);
            STATIC_REQUIRE(std::is_standard_layout_v<V>);
        }
    }

    {
        struct S
        {
        public:
            int i;
            int j;
        };
        static_assert(std::is_standard_layout_v<S>);
        STATIC_REQUIRE(std::is_standard_layout_v<yk::detail::make_variadic_union_t<S>>);
        STATIC_REQUIRE(std::is_standard_layout_v<yk::rvariant<S>>);
    }
    {
        struct S
        {
        public:
            int i;
        private:
            [[maybe_unused]] int j{};
        };
        static_assert(!std::is_standard_layout_v<S>);
        // ReSharper disable once CppStaticAssertFailure
        STATIC_REQUIRE(!std::is_standard_layout_v<yk::detail::make_variadic_union_t<S>>);
        // ReSharper disable once CppStaticAssertFailure
        STATIC_REQUIRE(!std::is_standard_layout_v<yk::rvariant<S>>);
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
            STATIC_REQUIRE(std::is_standard_layout_v<Base>);
        }
        {
            STATIC_REQUIRE(!std::is_trivially_copy_constructible_v<V>);
            STATIC_REQUIRE(std::is_trivially_move_constructible_v<V>);
            STATIC_REQUIRE(!std::is_trivially_copy_assignable_v<V>); // variant requires TCC && TCA && TD
            STATIC_REQUIRE(std::is_trivially_move_assignable_v<V>);
            STATIC_REQUIRE(std::is_trivially_destructible_v<V>);
            STATIC_REQUIRE(!std::is_trivially_copyable_v<V>);
            STATIC_REQUIRE(std::is_standard_layout_v<V>);
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
        STATIC_REQUIRE(std::is_standard_layout_v<S>);

        using V = yk::rvariant<S>;
        STATIC_REQUIRE( std::is_copy_constructible_v<V>);
        STATIC_REQUIRE(!std::is_trivially_copy_constructible_v<V>);

        STATIC_REQUIRE(std::is_move_constructible_v<V>);
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<V>);

        //STATIC_REQUIRE(std::is_trivially_copy_assignable_v<V>);
        STATIC_REQUIRE(std::is_trivially_move_assignable_v<V>);
        STATIC_REQUIRE(std::is_trivially_destructible_v<V>);
        STATIC_REQUIRE(!std::is_trivially_copyable_v<V>);
        STATIC_REQUIRE(std::is_standard_layout_v<V>);
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
        STATIC_REQUIRE(std::is_standard_layout_v<VD>);

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
        STATIC_REQUIRE(std::is_standard_layout_v<VD>);

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
        STATIC_REQUIRE(std::is_standard_layout_v<VD>);

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
        STATIC_REQUIRE(std::is_standard_layout_v<VD>);

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

TEST_CASE("variant_index_t", "[detail]")
{
    // NOLINTBEGIN(modernize-use-integer-sign-comparison)
    STATIC_REQUIRE(static_cast<std::size_t>(static_cast<yk::detail::variant_index_t<126>>(-1)) == std::variant_npos);
    STATIC_REQUIRE(static_cast<std::size_t>(static_cast<yk::detail::variant_index_t<127>>(-1)) == std::variant_npos);
    STATIC_REQUIRE(static_cast<std::size_t>(static_cast<yk::detail::variant_index_t<128>>(-1)) == std::variant_npos);
    // NOLINTEND(modernize-use-integer-sign-comparison)

    STATIC_REQUIRE(yk::detail::valueless_bias<false>(static_cast<yk::detail::variant_index_t<126>>(125)) == 126);
    STATIC_REQUIRE(yk::detail::valueless_bias<false>(static_cast<yk::detail::variant_index_t<127>>(126)) == 127);
    STATIC_REQUIRE(yk::detail::valueless_bias<false>(static_cast<yk::detail::variant_index_t<128>>(127)) == 128);
}

// --------------------------------------------

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

// --------------------------------------------

TEST_CASE("rvariant.rvariant.general", "[wrapper]")
{
    // [rvariant.rvariant.general]
    //yk::rvariant<int, yk::recursive_wrapper<int>>{}; // hard error
    //yk::rvariant<yk::recursive_wrapper<int>, int>{}; // hard error
    (void)yk::rvariant<yk::recursive_wrapper<int>, yk::recursive_wrapper<int>>{}; // ok
    //struct MyAllocator : std::allocator<int> {};
    //yk::rvariant<yk::recursive_wrapper<int>, yk::recursive_wrapper<int, MyAllocator>>{}; // hard error
}

TEST_CASE("special member function resolution")
{
    {
        struct NCC : SMF_Logger
        {
            NCC() = default;
            NCC(NCC const&) = delete;
            NCC& operator=(NCC const&) = default;
            NCC(NCC&&) = default;
            NCC& operator=(NCC&&) = default;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NCC>);
        STATIC_REQUIRE(!std::is_copy_constructible_v<NCC>);
        STATIC_REQUIRE( std::is_copy_assignable_v<NCC>);
        STATIC_REQUIRE( std::is_move_constructible_v<NCC>);
        STATIC_REQUIRE( std::is_move_assignable_v<NCC>);

        using V_NCC = yk::rvariant<NCC>;
        V_NCC temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NCC>);
        CHECK(yk::get<0>(V_NCC{}).log == "DC ");
        STATIC_CHECK(!std::is_copy_constructible_v<V_NCC>);
        STATIC_CHECK(!std::is_copy_assignable_v<V_NCC>); // strengthened; https://eel.is/c++draft/variant.assign#5
        STATIC_CHECK( std::is_move_constructible_v<V_NCC>);
        CHECK(yk::get<0>(V_NCC{std::move(temp)}).log == "MC "); // construction from xvalue
        STATIC_CHECK( std::is_move_assignable_v<V_NCC>);
        CHECK(yk::get<0>(V_NCC{} = V_NCC{}).log == "DC MA ");
    }
    {
        struct NMC : SMF_Logger
        {
            NMC() = default;
            NMC(NMC const&) = default;
            NMC& operator=(NMC const&) = default;
            NMC(NMC&&) = delete;
            NMC& operator=(NMC&&) = default;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NMC>);
        STATIC_REQUIRE( std::is_copy_constructible_v<NMC>);
        STATIC_REQUIRE( std::is_copy_assignable_v<NMC>);
        STATIC_REQUIRE(!std::is_move_constructible_v<NMC>);
        STATIC_REQUIRE( std::is_move_assignable_v<NMC>);

        using V_NMC = yk::rvariant<NMC>;
        V_NMC temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NMC>);
        CHECK(yk::get<0>(V_NMC{}).log == "DC ");
        STATIC_CHECK( std::is_copy_constructible_v<V_NMC>);
        CHECK(yk::get<0>(V_NMC{temp}).log == "CC ");
        STATIC_CHECK( std::is_copy_assignable_v<V_NMC>);
        CHECK(yk::get<0>(V_NMC{} = temp).log == "DC CA ");
        STATIC_CHECK( std::is_move_constructible_v<V_NMC>); // falls back to CC; https://eel.is/c++draft/variant.ctor#10
        CHECK(yk::get<0>(V_NMC{std::move(temp)}).log == "CC ");
        STATIC_CHECK( std::is_move_assignable_v<V_NMC>);
        CHECK(yk::get<0>(V_NMC{} = V_NMC{}).log == "DC CA "); // falls back to CA; https://eel.is/c++draft/variant.assign#7
    }

    {
        struct NCA : SMF_Logger
        {
            NCA() = default;
            NCA(NCA const&) = default;
            NCA& operator=(NCA const&) = delete;
            NCA(NCA&&) = default;
            NCA& operator=(NCA&&) = default;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NCA>);
        STATIC_REQUIRE( std::is_copy_constructible_v<NCA>);
        STATIC_REQUIRE(!std::is_copy_assignable_v<NCA>);
        STATIC_REQUIRE( std::is_move_constructible_v<NCA>);
        STATIC_REQUIRE( std::is_move_assignable_v<NCA>);

        using V_NCA = yk::rvariant<NCA>;
        V_NCA temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NCA>);
        CHECK(yk::get<0>(V_NCA{}).log == "DC ");
        STATIC_CHECK( std::is_copy_constructible_v<V_NCA>);
        CHECK(yk::get<0>(V_NCA{temp}).log == "CC ");
        STATIC_CHECK(!std::is_copy_assignable_v<V_NCA>);
        STATIC_CHECK( std::is_move_constructible_v<V_NCA>);
        CHECK(yk::get<0>(V_NCA{std::move(temp)}).log == "MC "); // construction from xvalue
        STATIC_CHECK( std::is_move_assignable_v<V_NCA>);
        CHECK(yk::get<0>(V_NCA{} = V_NCA{}).log == "DC MA ");
    }
    {
        struct NMA : SMF_Logger
        {
            NMA() = default;
            NMA(NMA const&) = default;
            NMA& operator=(NMA const&) = default;
            NMA(NMA&&) = default;
            NMA& operator=(NMA&&) = delete;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NMA>);
        STATIC_REQUIRE( std::is_copy_constructible_v<NMA>);
        STATIC_REQUIRE( std::is_copy_assignable_v<NMA>);
        STATIC_REQUIRE( std::is_move_constructible_v<NMA>);
        STATIC_REQUIRE(!std::is_move_assignable_v<NMA>);

        using V_NMA = yk::rvariant<NMA>;
        V_NMA temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NMA>);
        CHECK(yk::get<0>(V_NMA{}).log == "DC ");
        STATIC_CHECK( std::is_copy_constructible_v<V_NMA>);
        CHECK(yk::get<0>(V_NMA{temp}).log == "CC ");
        STATIC_CHECK( std::is_copy_assignable_v<V_NMA>);
        CHECK(yk::get<0>(V_NMA{} = temp).log == "DC CA ");
        STATIC_CHECK( std::is_move_constructible_v<V_NMA>);
        CHECK(yk::get<0>(V_NMA{std::move(temp)}).log == "MC "); // construction from xvalue
        STATIC_CHECK( std::is_move_assignable_v<V_NMA>); // falls back to CA; https://eel.is/c++draft/variant.assign#7
        CHECK(yk::get<0>(V_NMA{} = V_NMA{}).log == "DC CA ");
    }

    {
        struct NCC_NCA : SMF_Logger
        {
            NCC_NCA() = default;
            NCC_NCA(NCC_NCA const&) = delete;
            NCC_NCA& operator=(NCC_NCA const&) = delete;
            NCC_NCA(NCC_NCA&&) = default;
            NCC_NCA& operator=(NCC_NCA&&) = default;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NCC_NCA>);
        STATIC_REQUIRE(!std::is_copy_constructible_v<NCC_NCA>);
        STATIC_REQUIRE(!std::is_copy_assignable_v<NCC_NCA>);
        STATIC_REQUIRE( std::is_move_constructible_v<NCC_NCA>);
        STATIC_REQUIRE( std::is_move_assignable_v<NCC_NCA>);

        using V_NCC_NCA = yk::rvariant<NCC_NCA>;
        V_NCC_NCA temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NCC_NCA>);
        CHECK(yk::get<0>(V_NCC_NCA{}).log == "DC ");
        STATIC_CHECK(!std::is_copy_constructible_v<V_NCC_NCA>);
        STATIC_CHECK(!std::is_copy_assignable_v<V_NCC_NCA>);
        STATIC_CHECK( std::is_move_constructible_v<V_NCC_NCA>);
        CHECK(yk::get<0>(V_NCC_NCA{std::move(temp)}).log == "MC "); // construction from xvalue
        STATIC_CHECK( std::is_move_assignable_v<V_NCC_NCA>);
        CHECK(yk::get<0>(V_NCC_NCA{} = V_NCC_NCA{}).log == "DC MA ");
    }
    {
        struct NMC_NMA : SMF_Logger
        {
            NMC_NMA() = default;
            NMC_NMA(NMC_NMA const&) = default;
            NMC_NMA& operator=(NMC_NMA const&) = default;
            NMC_NMA(NMC_NMA&&) = delete;
            NMC_NMA& operator=(NMC_NMA&&) = delete;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NMC_NMA>);
        STATIC_REQUIRE( std::is_copy_constructible_v<NMC_NMA>);
        STATIC_REQUIRE( std::is_copy_assignable_v<NMC_NMA>);
        STATIC_REQUIRE(!std::is_move_constructible_v<NMC_NMA>);
        STATIC_REQUIRE(!std::is_move_assignable_v<NMC_NMA>);

        using V_NMC_NMA = yk::rvariant<NMC_NMA>;
        V_NMC_NMA temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NMC_NMA>);
        CHECK(yk::get<0>(V_NMC_NMA{}).log == "DC ");
        STATIC_CHECK( std::is_copy_constructible_v<V_NMC_NMA>);
        CHECK(yk::get<0>(V_NMC_NMA{temp}).log == "CC ");
        STATIC_CHECK( std::is_copy_assignable_v<V_NMC_NMA>);
        CHECK(yk::get<0>(V_NMC_NMA{} = temp).log == "DC CA ");
        STATIC_CHECK( std::is_move_constructible_v<V_NMC_NMA>); // falls back to CC; https://eel.is/c++draft/variant.ctor#10
        CHECK(yk::get<0>(V_NMC_NMA{std::move(temp)}).log == "CC "); // construction from xvalue
        STATIC_CHECK( std::is_move_assignable_v<V_NMC_NMA>); // falls back to CA; https://eel.is/c++draft/variant.assign#7
        CHECK(yk::get<0>(V_NMC_NMA{} = V_NMC_NMA{}).log == "DC CA ");
    }

    {
        struct NCC_NMC_NMA : SMF_Logger
        {
            NCC_NMC_NMA() = default;
            NCC_NMC_NMA(NCC_NMC_NMA const&) = delete;
            NCC_NMC_NMA& operator=(NCC_NMC_NMA const&) = default;
            NCC_NMC_NMA(NCC_NMC_NMA&&) = delete;
            NCC_NMC_NMA& operator=(NCC_NMC_NMA&&) = delete;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NCC_NMC_NMA>);
        STATIC_REQUIRE(!std::is_copy_constructible_v<NCC_NMC_NMA>);
        STATIC_REQUIRE( std::is_copy_assignable_v<NCC_NMC_NMA>);
        STATIC_REQUIRE(!std::is_move_constructible_v<NCC_NMC_NMA>);
        STATIC_REQUIRE(!std::is_move_assignable_v<NCC_NMC_NMA>);

        using V_NCC_NMC_NMA = yk::rvariant<NCC_NMC_NMA>;
        V_NCC_NMC_NMA temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NCC_NMC_NMA>);
        CHECK(yk::get<0>(V_NCC_NMC_NMA{}).log == "DC ");
        STATIC_CHECK(!std::is_copy_constructible_v<V_NCC_NMC_NMA>);
        STATIC_CHECK(!std::is_copy_assignable_v<V_NCC_NMC_NMA>); // strengthened; https://eel.is/c++draft/variant.assign#5
        STATIC_CHECK(!std::is_move_constructible_v<V_NCC_NMC_NMA>); // falls back to CC(deleted)
        STATIC_CHECK(!std::is_move_assignable_v<V_NCC_NMC_NMA>); // falls back to CA(deleted)
    }
    {
        struct NCA_NMC_NMA : SMF_Logger
        {
            NCA_NMC_NMA() = default;
            NCA_NMC_NMA(NCA_NMC_NMA const&) = default;
            NCA_NMC_NMA& operator=(NCA_NMC_NMA const&) = delete;
            NCA_NMC_NMA(NCA_NMC_NMA&&) = delete;
            NCA_NMC_NMA& operator=(NCA_NMC_NMA&&) = delete;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NCA_NMC_NMA>);
        STATIC_REQUIRE( std::is_copy_constructible_v<NCA_NMC_NMA>);
        STATIC_REQUIRE(!std::is_copy_assignable_v<NCA_NMC_NMA>);
        STATIC_REQUIRE(!std::is_move_constructible_v<NCA_NMC_NMA>);
        STATIC_REQUIRE(!std::is_move_assignable_v<NCA_NMC_NMA>);

        using V_NMC_NMA = yk::rvariant<NCA_NMC_NMA>;
        V_NMC_NMA temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NMC_NMA>);
        CHECK(yk::get<0>(V_NMC_NMA{}).log == "DC ");
        STATIC_CHECK( std::is_copy_constructible_v<V_NMC_NMA>);
        CHECK(yk::get<0>(V_NMC_NMA{temp}).log == "CC ");
        STATIC_CHECK(!std::is_copy_assignable_v<V_NMC_NMA>);
        STATIC_CHECK( std::is_move_constructible_v<V_NMC_NMA>); // falls back to CC; https://eel.is/c++draft/variant.ctor#10
        CHECK(yk::get<0>(V_NMC_NMA{std::move(temp)}).log == "CC "); // construction from xvalue
        STATIC_CHECK(!std::is_move_assignable_v<V_NMC_NMA>); // falls back to CA(deleted)
    }

    {
        struct NCC_NCA_NMC_NMA : SMF_Logger
        {
            NCC_NCA_NMC_NMA() = default;
            NCC_NCA_NMC_NMA(NCC_NCA_NMC_NMA const&) = delete;
            NCC_NCA_NMC_NMA(NCC_NCA_NMC_NMA&&) = delete;
            NCC_NCA_NMC_NMA& operator=(NCC_NCA_NMC_NMA const&) = delete;
            NCC_NCA_NMC_NMA& operator=(NCC_NCA_NMC_NMA&&) = delete;
        };
        STATIC_REQUIRE( std::is_default_constructible_v<NCC_NCA_NMC_NMA>);
        STATIC_REQUIRE(!std::is_copy_constructible_v<NCC_NCA_NMC_NMA>);
        STATIC_REQUIRE(!std::is_move_constructible_v<NCC_NCA_NMC_NMA>);
        STATIC_REQUIRE(!std::is_copy_assignable_v<NCC_NCA_NMC_NMA>);
        STATIC_REQUIRE(!std::is_move_assignable_v<NCC_NCA_NMC_NMA>);

        using V_NCC_NCA_NMC_NMA = yk::rvariant<NCC_NCA_NMC_NMA>;
        V_NCC_NCA_NMC_NMA temp;

        STATIC_CHECK( std::is_default_constructible_v<V_NCC_NCA_NMC_NMA>);
        CHECK(yk::get<0>(V_NCC_NCA_NMC_NMA{}).log == "DC ");
        STATIC_CHECK(!std::is_copy_constructible_v<V_NCC_NCA_NMC_NMA>);
        STATIC_CHECK(!std::is_move_constructible_v<V_NCC_NCA_NMC_NMA>);
        STATIC_CHECK(!std::is_copy_assignable_v<V_NCC_NCA_NMC_NMA>);
        STATIC_CHECK(!std::is_move_assignable_v<V_NCC_NCA_NMC_NMA>);
    }
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

        // NB: bit_cast from union type is not allowed in constant expression
        using V = yk::rvariant<int>;
        alignas(V) std::array<std::byte, sizeof(V)> storage;
        {
            V arbitrary_value(42);
            storage = std::bit_cast<decltype(storage)>(arbitrary_value);
        }
        V* v_ptr = new (&storage) V; // default-initialize
        int const default_constructed_value = yk::get<int>(*v_ptr); // MUST be value-initialized as per https://eel.is/c++draft/variant.ctor#3
        v_ptr->~V();

        CHECK(default_constructed_value == 0);
    }

    {
        yk::rvariant<int> var;
        STATIC_REQUIRE(std::is_nothrow_default_constructible_v<yk::rvariant<int>>);
        REQUIRE(var.valueless_by_exception() == false);
        CHECK(var.index() == 0);
    }
    {
        struct S
        {
            S() noexcept(false) {}
        };
        // ReSharper disable once CppStaticAssertFailure
        STATIC_REQUIRE(!std::is_nothrow_default_constructible_v<S>);
        // ReSharper disable once CppStaticAssertFailure
        STATIC_REQUIRE(!std::is_nothrow_default_constructible_v<yk::rvariant<S>>);
    }
    {
        struct S
        {
            S() = delete;
        };
        STATIC_REQUIRE(!std::is_default_constructible_v<S>);
        STATIC_REQUIRE(!std::is_default_constructible_v<yk::rvariant<S>>);
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
        REQUIRE(b.valueless_by_exception() == false);
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
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, MC_Thrower> a(valueless);
        CHECK(a.valueless_by_exception() == true);
    }
}

TEST_CASE("move construction")
{
    {
        STATIC_REQUIRE(std::is_trivially_move_constructible_v<yk::rvariant<int>>);
        yk::rvariant<int, double> a;
        yk::rvariant<int, double> b(std::move(a));
        REQUIRE(b.valueless_by_exception() == false);
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
            S& operator=(S&&) = delete;
        };
        static_assert(!std::is_move_assignable_v<S>);
        yk::rvariant<S> a;
        static_assert(std::is_move_constructible_v<yk::rvariant<S>>);
        REQUIRE_THROWS(yk::rvariant<S>(std::move(a)));
    }

    {
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, MC_Thrower> a(std::move(valueless));
        CHECK(a.valueless_by_exception() == true);
    }
}

TEST_CASE("generic construction")
{
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(var.valueless_by_exception() == false);
        CHECK(var.index() == 0);
    }
    {
        yk::rvariant<int, float> var = 3.14f;
        REQUIRE(var.valueless_by_exception() == false);
        CHECK(var.index() == 1);
    }
}

TEST_CASE("in_place_index construction")
{
    {
        yk::rvariant<int, float> var(std::in_place_index<0>, 42);
        REQUIRE(var.valueless_by_exception() == false);
        CHECK(var.index() == 0);
    }
    {
        yk::rvariant<int, float> var(std::in_place_index<1>, 3.14f);
        REQUIRE(var.valueless_by_exception() == false);
        CHECK(var.index() == 1);
    }
    {
        yk::rvariant<std::vector<int>> var(std::in_place_index<0>, {3, 1, 4});
    }
}

TEST_CASE("in_place_type construction")
{
    {
        yk::rvariant<int, float> var(std::in_place_type<int>, 42);
        REQUIRE(var.valueless_by_exception() == false);
        CHECK(var.index() == 0);
    }
    {
        yk::rvariant<int, float> var(std::in_place_type<float>, 3.14f);
        REQUIRE(var.valueless_by_exception() == false);
        CHECK(var.index() == 1);
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
        YK_REQUIRE_STATIC_NOTHROW(a = b);  // different alternative
        REQUIRE(a.index() == 1);
        YK_REQUIRE_STATIC_NOTHROW(a = b);  // same alternative
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
        YK_REQUIRE_STATIC_NOTHROW(a = b);  // different alternative; use copy constructor
        YK_REQUIRE_STATIC_NOTHROW(a = b);  // same alternative; directly use copy assignment
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
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, MC_Thrower> a;
        a = valueless;
        CHECK(a.valueless_by_exception() == true);
    }

    // NOLINTEND(modernize-use-equals-default)
}

TEST_CASE("move assignment")
{
    // trivial case
    {
        yk::rvariant<int, float> a = 42, b = 3.14f;
        YK_REQUIRE_STATIC_NOTHROW(a = std::move(b));  // different alternative
        REQUIRE(a.index() == 1);
    }
    {
        yk::rvariant<int, float> a = 42, b = 33 - 4;
        YK_REQUIRE_STATIC_NOTHROW(a = std::move(b));  // same alternative
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
            YK_CHECK_STATIC_NOTHROW(a = std::move(b));  // different alternative; use move constructor
            CHECK(a.index() == 0);
        }
        {
            yk::rvariant<S, int> a, b;
            YK_CHECK_STATIC_NOTHROW(a = std::move(b));  // same alternative; directly use move assignment
            CHECK(a.index() == 0);
        }
    }

    {
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, MC_Thrower> a;
        a = std::move(valueless);
        CHECK(a.valueless_by_exception() == true);
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
        yk::rvariant<int, MC_Thrower> a;
        REQUIRE_THROWS_AS(a = MC_Thrower{}, MC_Thrower::exception);
        CHECK(a.valueless_by_exception() == true);
    }
    {
        yk::rvariant<int, MC_Thrower> a;
        YK_REQUIRE_STATIC_NOTHROW(a = MC_Thrower::non_throwing);
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<int, MC_Thrower> a;
        REQUIRE_THROWS_AS(a = MC_Thrower::throwing, MC_Thrower::exception);
        CHECK(a.valueless_by_exception() == true);
    }
    {
        yk::rvariant<int, MC_Thrower> a;
        STATIC_REQUIRE(noexcept(a = MC_Thrower::potentially_throwing) == false);
        REQUIRE_NOTHROW(a = MC_Thrower::potentially_throwing);
        CHECK(a.valueless_by_exception() == false);
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
        YK_REQUIRE_STATIC_NOTHROW(a.emplace<int>(12));
        YK_REQUIRE_STATIC_NOTHROW(a.emplace<0>(12));
    }
    {
        yk::rvariant<std::vector<int>> a;
        a.emplace<std::vector<int>>({3, 1, 4});
        a.emplace<0>({3, 1, 4});
    }
    {
        yk::rvariant<int, float> a = 42;
        a.emplace<1>(3.14f);
        CHECK(a.index() == 1);
    }

    {
        // NOLINTBEGIN(modernize-use-equals-default)
        {
            struct S
            {
                S() noexcept(false) {} // potentially-throwing
            };
            using V = yk::rvariant<int, S>;
            {
                V v(std::in_place_type<int>);
                v.emplace<S>(); // type-changing & no args; test T{}
            }
            {
                V v(std::in_place_type<S>);
                v.emplace<S>(); // non-type-changing & no args; test T{}
            }
        }
        {
            struct StrangeS
            {
                StrangeS() noexcept(false) {} // potentially-throwing
                StrangeS(StrangeS&&) noexcept {} // not trivial
                StrangeS(StrangeS const&) = default; // trivial
                StrangeS& operator=(StrangeS&&) noexcept { return *this; } // not trivial
                StrangeS& operator=(StrangeS const&) = default; // trivial
            };
            using V = yk::rvariant<int, StrangeS>;
            {
                V v(std::in_place_type<int>);
                v.emplace<StrangeS>(); // type-changing & no args; test T{}
            }
            {
                V v(std::in_place_type<StrangeS>);
                v.emplace<StrangeS>(); // non-type-changing & no args; test T{}
            }
        }
        // NOLINTEND(modernize-use-equals-default)
    }

    // ReSharper disable CppStaticAssertFailure

    struct BigType : Thrower_base
    {
        std::byte dummy[yk::detail::never_valueless_trivial_size_limit + 1]{};
    };
    STATIC_REQUIRE(!yk::detail::is_never_valueless_v<BigType>);

    // non type-changing
    STATIC_REQUIRE(!is_never_valueless<yk::rvariant<BigType>>);
    {
        yk::rvariant<BigType> a;
        REQUIRE_THROWS_AS(a.emplace<0>(BigType::throwing), BigType::exception);
        CHECK(a.valueless_by_exception() == true);
    }
    // type-changing
    STATIC_REQUIRE(!is_never_valueless<yk::rvariant<int, BigType>>);
    {
        yk::rvariant<int, BigType> a;
        REQUIRE_THROWS_AS(a.emplace<1>(BigType::throwing), BigType::exception);
        CHECK(a.valueless_by_exception() == true);
    }

    // non type-changing
    STATIC_REQUIRE(is_never_valueless<yk::rvariant<Non_Thrower>>);
    {
        yk::rvariant<Non_Thrower> a;
        YK_REQUIRE_STATIC_NOTHROW(a.emplace<0>(Non_Thrower{}));
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<Non_Thrower> a;
        YK_REQUIRE_STATIC_NOTHROW(a.emplace<0>(Non_Thrower::non_throwing));
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<Non_Thrower> a;
        REQUIRE_THROWS_AS(a.emplace<0>(Non_Thrower::throwing), Non_Thrower::exception);
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<Non_Thrower> a;
        REQUIRE_NOTHROW(a.emplace<0>(Non_Thrower::potentially_throwing));
        CHECK(a.valueless_by_exception() == false);
    }

    // type-changing
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<Non_Thrower>);
    STATIC_REQUIRE(is_never_valueless<yk::rvariant<int, Non_Thrower>>);
    {
        yk::rvariant<int, Non_Thrower> a;
        YK_REQUIRE_STATIC_NOTHROW(a.emplace<1>(Non_Thrower{}));
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<int, Non_Thrower> a;
        YK_REQUIRE_STATIC_NOTHROW(a.emplace<1>(Non_Thrower::non_throwing));
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<int, Non_Thrower> a;
        REQUIRE_THROWS_AS(a.emplace<1>(Non_Thrower::throwing), Non_Thrower::exception);
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<int, Non_Thrower> a;
        REQUIRE_NOTHROW(a.emplace<1>(Non_Thrower::potentially_throwing));
        CHECK(a.valueless_by_exception() == false);
    }

    // non type-changing
    STATIC_REQUIRE(!std::is_nothrow_move_constructible_v<MC_Thrower>);
    STATIC_REQUIRE(!is_never_valueless<yk::rvariant<MC_Thrower>>);
    {
        yk::rvariant<MC_Thrower> a;
        REQUIRE_THROWS_AS(a.emplace<0>(MC_Thrower{}), MC_Thrower::exception);
        CHECK(a.valueless_by_exception() == true);
    }
    {
        yk::rvariant<MC_Thrower> a;
        YK_REQUIRE_STATIC_NOTHROW(a.emplace<0>(MC_Thrower::non_throwing));
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<MC_Thrower> a;
        REQUIRE_THROWS_AS(a.emplace<0>(MC_Thrower::throwing), MC_Thrower::exception);
        CHECK(a.valueless_by_exception() == true);
    }
    {
        yk::rvariant<MC_Thrower> a;
        REQUIRE_NOTHROW(a.emplace<0>(MC_Thrower::potentially_throwing));
        CHECK(a.valueless_by_exception() == false);
    }

    // type-changing
    STATIC_REQUIRE(!std::is_nothrow_move_constructible_v<MC_Thrower>);
    STATIC_REQUIRE(!is_never_valueless<yk::rvariant<int, MC_Thrower>>);
    {
        yk::rvariant<int, MC_Thrower> a;
        REQUIRE_THROWS_AS(a.emplace<1>(MC_Thrower{}), MC_Thrower::exception);
        CHECK(a.valueless_by_exception() == true);
    }
    {
        yk::rvariant<int, MC_Thrower> a;
        YK_REQUIRE_STATIC_NOTHROW(a.emplace<1>(MC_Thrower::non_throwing));
        CHECK(a.valueless_by_exception() == false);
    }
    {
        yk::rvariant<int, MC_Thrower> a;
        REQUIRE_THROWS_AS(a.emplace<1>(MC_Thrower::throwing), MC_Thrower::exception);
        CHECK(a.valueless_by_exception() == true);
    }
    {
        yk::rvariant<int, MC_Thrower> a;
        REQUIRE_NOTHROW(a.emplace<1>(MC_Thrower::potentially_throwing));
        CHECK(a.valueless_by_exception() == false);
    }

    STATIC_REQUIRE(yk::detail::is_never_valueless_v<yk::recursive_wrapper<int>>);
    STATIC_REQUIRE(is_never_valueless<yk::rvariant<yk::recursive_wrapper<int>>>);

    // ReSharper restore CppStaticAssertFailure
}


namespace {

struct any_consumer
{
    [[maybe_unused]] char dummy{}; // MSVC bug: MSVC falsely tries to access "uninitialized symbol" without this workaround

    constexpr any_consumer() {}  // NOLINT(modernize-use-equals-default)
    constexpr ~any_consumer() {}  // NOLINT(modernize-use-equals-default)
    constexpr any_consumer(any_consumer const&) {}
    constexpr any_consumer(any_consumer&&) noexcept(false) {}
    constexpr any_consumer& operator=(any_consumer const&) = delete;
    constexpr any_consumer& operator=(any_consumer&&) = delete;

    template<class T>
        requires (!std::is_same_v<std::remove_cvref_t<T>, any_consumer>)
    constexpr explicit any_consumer(T&&) {}

    template<class T>
        requires (!std::is_same_v<std::remove_cvref_t<T>, any_consumer>)
    constexpr any_consumer& operator=(T&&) { return *this; }
};

} // anonymous

TEST_CASE("self emplace")
{
    // Valueless instance never triggers the self-emplace assertion
    STATIC_REQUIRE([] consteval {
        auto v = yk::detail::make_valueless<int, any_consumer>();
        v.emplace<any_consumer>(v);
        return true;
    }());
    REQUIRE([] {
        auto v = yk::detail::make_valueless<int, any_consumer>();
        v.emplace<any_consumer>(v);
        return true;
    }());
    // Triggers self-emplace assertion
    //REQUIRE([] {
    //    yk::rvariant<int, any_consumer> v;
    //    v.emplace<any_consumer>(v);
    //    return true;
    //}());

    // Valueless case is UNTESTABLE for generic assignment.
    // There's no way to "assign" valueless object because
    // self-assignment always resolves to the move assignment
    // operator of `rvariant` itself (not the generic one).
}

TEST_CASE("swap")
{
    {
        STATIC_REQUIRE(yk::core::is_trivially_swappable_v<int>);
        yk::rvariant<int> a = 33, b = 4;
        YK_REQUIRE_STATIC_NOTHROW(a.swap(b));
        CHECK(yk::get<0>(a) == 4);
        CHECK(yk::get<0>(b) == 33);
    }
    {
        yk::rvariant<int, float> a = 42, b = 3.14f;
        YK_REQUIRE_STATIC_NOTHROW(a.swap(b));
        CHECK(yk::get<1>(a) == 3.14f);
        CHECK(yk::get<0>(b) == 42);
    }
    {
        struct S
        {
            S() = default;
            S(S const&) = default;
            S(S&&) noexcept(false) {}
            S& operator=(S const&) = default;
            S& operator=(S&&) noexcept(false) { return *this; }
        };

        STATIC_REQUIRE(!yk::core::is_trivially_swappable_v<S>);
        yk::rvariant<int, S> a{42}, b{S{}};
        REQUIRE_NOTHROW(a.swap(b));
        CHECK(yk::holds_alternative<S>(a));
        CHECK(yk::holds_alternative<int>(b));
    }

    {
        yk::rvariant<int, MC_Thrower> a = make_valueless<int>();
        yk::rvariant<int, MC_Thrower> b;
        CHECK( a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
        try {
            a.swap(b);
        } catch (...) {  // NOLINT(bugprone-empty-catch)
        }
        CHECK(!a.valueless_by_exception());
        CHECK( b.valueless_by_exception());
    }
    {
        yk::rvariant<int, MC_Thrower> a;
        yk::rvariant<int, MC_Thrower> b = make_valueless<int>();
        CHECK(!a.valueless_by_exception());
        CHECK( b.valueless_by_exception());
        try {
            a.swap(b);
        } catch (...) {  // NOLINT(bugprone-empty-catch)
        }
        CHECK( a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
    }
    {
        yk::rvariant<int, MC_Thrower> a = make_valueless<int>();
        yk::rvariant<int, MC_Thrower> b = make_valueless<int>();
        CHECK(a.valueless_by_exception());
        CHECK(b.valueless_by_exception());
        CHECK_NOTHROW(a.swap(b));
        CHECK(a.valueless_by_exception());
        CHECK(b.valueless_by_exception());
    }
    {
        yk::rvariant<int, MC_Thrower> a = make_valueless<int>();
        yk::rvariant<int, MC_Thrower> b(std::in_place_type<MC_Thrower>);
        CHECK( a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
        try {
            a.swap(b);
        } catch (...) {  // NOLINT(bugprone-empty-catch)
        }
        CHECK( a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
    }
    {
        yk::rvariant<int, MC_Thrower> a(std::in_place_type<int>);
        yk::rvariant<int, MC_Thrower> b(std::in_place_type<MC_Thrower>);
        CHECK(!a.valueless_by_exception());
        CHECK(!b.valueless_by_exception());
        try {
            a.swap(b);
        } catch (...) {  // NOLINT(bugprone-empty-catch)
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
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        CHECK(!yk::holds_alternative<int>(valueless));
        CHECK(!yk::holds_alternative<MC_Thrower>(valueless));
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
            S(int, double) {}
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
        // ReSharper disable once CppStaticAssertFailure
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

namespace {

struct noneq_three_way_thrower
{
    struct exception {};
    bool operator==(noneq_three_way_thrower const&) = delete;
    bool operator<(noneq_three_way_thrower const&) const { return false; }
    std::strong_ordering operator<=>(noneq_three_way_thrower const&) const { throw exception{}; }  // NOLINT(hicpp-exception-baseclass)
};

struct eq_three_way_thrower
{
    struct exception {};
    bool operator==(eq_three_way_thrower const&) const { return true; }
    bool operator<(eq_three_way_thrower const&) const { return false; }
    bool operator>(eq_three_way_thrower const&) const { return false; }
    bool operator<=(eq_three_way_thrower const&) const { return true; }
    bool operator>=(eq_three_way_thrower const&) const { return true; }
    std::strong_ordering operator<=>(eq_three_way_thrower const&) const { throw exception{}; }  // NOLINT(hicpp-exception-baseclass)
};

} // anonymous

TEST_CASE("relational operators")
{
    {
        REQUIRE_NOTHROW(noneq_three_way_thrower{} < noneq_three_way_thrower{});
        STATIC_REQUIRE(!std::is_invocable_r_v<bool, std::equal_to<>, noneq_three_way_thrower const&, noneq_three_way_thrower const&>);
        STATIC_REQUIRE(!yk::core::relop_bool_expr_v<std::equal_to<>, noneq_three_way_thrower>);
        REQUIRE_THROWS_AS(noneq_three_way_thrower{} <=> noneq_three_way_thrower{}, noneq_three_way_thrower::exception);

        // std::variant on GCC/Clang/MSVC all fails to evaluate these.
        // TODO: Is this a bug on their implementation?
        {
            using V = yk::rvariant<noneq_three_way_thrower>;
            CHECK_NOTHROW(V{} < V{});
            STATIC_REQUIRE(!std::is_invocable_r_v<bool, std::equal_to<>, V const&, V const&>);
            STATIC_CHECK(!yk::core::relop_bool_expr_v<std::equal_to<>, V>);
        }
    }

    // std::variant on GCC/Clang/MSVC all fails to evaluate these.
    // TODO: Is this a bug on their implementation?

    {
        STATIC_REQUIRE(std::is_invocable_r_v<bool, std::equal_to<>, eq_three_way_thrower const&, eq_three_way_thrower const&>);
        STATIC_REQUIRE(yk::core::relop_bool_expr_v<std::equal_to<>, eq_three_way_thrower>);
        REQUIRE_NOTHROW(eq_three_way_thrower{} < eq_three_way_thrower{});
        REQUIRE_THROWS_AS(eq_three_way_thrower{} <=> eq_three_way_thrower{}, eq_three_way_thrower::exception);

        {
            using V = yk::rvariant<eq_three_way_thrower>;
            STATIC_REQUIRE(std::is_invocable_r_v<bool, std::equal_to<>, V const&, V const&>);
            STATIC_CHECK(yk::core::relop_bool_expr_v<std::equal_to<>, V>);
            CHECK_NOTHROW(V{} == V{});
            CHECK_NOTHROW(V{} != V{});
            CHECK_NOTHROW(V{} <  V{}); // make sure <=> is not selected
            CHECK_NOTHROW(V{} >  V{}); // make sure <=> is not selected
            CHECK_NOTHROW(V{} <= V{}); // make sure <=> is not selected
            CHECK_NOTHROW(V{} >= V{}); // make sure <=> is not selected
            CHECK_THROWS_AS(V{} <=> V{}, eq_three_way_thrower::exception);
        }
    }

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
        yk::rvariant<int, MC_Thrower> valueless = make_valueless<int>();
        yk::rvariant<int, MC_Thrower> a;

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
            3.0141,
            2.0718,
            1.0414,
            1.0618,
        };
        std::ranges::sort(vec);
        CHECK(vec == std::vector<yk::rvariant<int, double>>{
            4,
            33,
            42,
            1.0414,
            1.0618,
            2.0718,
            3.0141,
        });
    }
}

namespace {

template<class T>
[[nodiscard]] constexpr std::size_t do_hash(T const& t) noexcept(yk::core::is_nothrow_hashable_v<T>)
{
    return std::hash<T>{}(t);
}

} // anonymous

TEST_CASE("rvariant.hash")
{
    {
        STATIC_REQUIRE(yk::core::is_hash_enabled_v<int>);
        STATIC_REQUIRE(yk::core::is_hash_enabled_v<yk::rvariant<int>>);
        CHECK(do_hash(yk::rvariant<int>(42)) == hash_value(yk::rvariant<int>(42)));
    }
    {
        struct NonExistent {};
        STATIC_REQUIRE(!yk::core::is_hash_enabled_v<NonExistent>);
        STATIC_REQUIRE(!yk::core::is_hash_enabled_v<yk::rvariant<NonExistent>>);
    }
    {
        STATIC_REQUIRE(yk::core::is_hash_enabled_v<HashForwarded<int>>);
        CHECK(std::hash<int>{}(42) == std::hash<HashForwarded<int>>{}(HashForwarded<int>(42)));
    }
    {
        STATIC_REQUIRE(yk::core::is_hash_enabled_v<yk::rvariant<HashForwarded<int>>>);
        yk::rvariant<HashForwarded<int>> v(HashForwarded<int>(42));
        CHECK(do_hash(v) == hash_value(v));
    }

    // heuristic tests; may fail on collisions
    {
        using Int = HashForwarded<int>;
        using V = yk::rvariant<int, Int>;
        CHECK(do_hash(V(std::in_place_type<int>, 42)) != do_hash(V(std::in_place_type<Int>, 42)));
    }
    {
        using V = yk::rvariant<int, unsigned>;
        CHECK(do_hash(V(std::in_place_type<int>, 42)) != do_hash(V(std::in_place_type<unsigned>, 42)));
    }
    {
        using V = yk::rvariant<int, long long>;
        CHECK(do_hash(V(std::in_place_type<int>, 42)) != do_hash(V(std::in_place_type<long long>, 42)));
    }
}

TEST_CASE("rvariant.hash", "[wrapper]")
{
    {
        STATIC_REQUIRE(yk::core::is_hash_enabled_v<yk::recursive_wrapper<int>>);
        CHECK(std::hash<int>{}(42) == std::hash<yk::recursive_wrapper<int>>{}(yk::recursive_wrapper<int>(42)));
        CHECK(std::hash<int>{}(42) == hash_value(yk::recursive_wrapper<int>(42)));
    }
    {
        yk::recursive_wrapper<int> a(42);
        auto b = std::move(a);
        REQUIRE(a.valueless_after_move());  // NOLINT(bugprone-use-after-move)
        CHECK(std::hash<yk::recursive_wrapper<int>>{}(a) == hash_value(a));
    }
}

// --------------------------------------------

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
