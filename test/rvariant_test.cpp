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


TEST_CASE("pack_indexing", "[lang_core]")
{
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int, float>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<1, int, float>, float>);
}

TEST_CASE("exactly_once", "[lang_core]")
{
    STATIC_REQUIRE(yk::detail::exactly_once_v<int, yk::detail::type_list<int, float>>);
    STATIC_REQUIRE_FALSE(yk::detail::exactly_once_v<int, yk::detail::type_list<int, int>>);
}

TEST_CASE("is_in", "[lang_core]")
{
    STATIC_REQUIRE(yk::detail::is_in_v<int, int, float>);
    STATIC_REQUIRE_FALSE(yk::detail::is_in_v<int, float>);
}

TEST_CASE("find_index", "[lang_core]")
{
    STATIC_REQUIRE(yk::detail::find_index_v<int, yk::detail::type_list<int, float, double>> == 0);
    STATIC_REQUIRE(yk::detail::find_index_v<float, yk::detail::type_list<int, float, double>> == 1);
    STATIC_REQUIRE(yk::detail::find_index_v<double, yk::detail::type_list<int, float, double>> == 2);
    STATIC_REQUIRE(yk::detail::find_index_v<int, yk::detail::type_list<float, double>> == yk::detail::find_index_npos);
}

TEST_CASE("pack_union", "[lang_core]")
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

TEST_CASE("compact_alternative")
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

TEST_CASE("storage")
{
    // Test cases for this type is VERY important; some compilers misunderstand relevant type traits
    struct NonExistent {};

    // NOLINTBEGIN(modernize-use-equals-default)
    {
        using T = int;
        using A = yk::detail::alternative<0, T>;
        {
            STATIC_REQUIRE(std::is_nothrow_default_constructible_v<A>);   // valueless

            STATIC_REQUIRE(std::is_trivially_copy_constructible_v<A>);
            STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<A>);

            STATIC_REQUIRE(std::is_trivially_move_constructible_v<A>);
            STATIC_REQUIRE(std::is_nothrow_move_constructible_v<A>);

            STATIC_REQUIRE(std::is_trivially_copy_assignable_v<A>);
            STATIC_REQUIRE(std::is_nothrow_copy_assignable_v<A>);

            STATIC_REQUIRE(std::is_trivially_move_assignable_v<A>);
            STATIC_REQUIRE(std::is_nothrow_move_assignable_v<A>);

            STATIC_REQUIRE(std::is_trivially_destructible_v<A>);
            STATIC_REQUIRE(std::is_nothrow_destructible_v<A>);

            STATIC_REQUIRE(std::is_trivially_copyable_v<A>);

            STATIC_REQUIRE(std::is_nothrow_default_constructible_v<A>); // default construct
            STATIC_REQUIRE(std::is_constructible_v<A, T>);
            STATIC_REQUIRE(!std::is_constructible_v<A, NonExistent>);
        }

        using VD = yk::detail::variadic_union<true, A>;
        STATIC_REQUIRE(std::is_same_v<yk::detail::variant_storage_for<T>, VD>);
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

// Note: Requires either (A) constexpr reinterpret_cast (C++26) or (B) std::default_construct_at
constexpr bool is_constexpr_default_construct_testable =
#if __cpp_lib_constexpr_new >= 202406L
    true;
#else
    false;
#endif

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

#if is_constexpr_default_construct_testable
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
    {
        struct S
        {
            S() = default;
            S(S const&) = default;
            S(S&&) = default;
            S& operator=(S const&) { return *this; }
            S& operator=(S&&) = default;
        };
        std::variant<S> a, b;

        // TODO: investigate why is this not even implemented in MSVC/STL.
        // I have no idea why this works; why is the defaulted operator NOT invalid for non-trivial storage?

        static_assert(!std::is_trivially_copy_assignable_v<S>);
        static_assert(!std::is_trivially_copy_assignable_v<std::variant<S>>);
        a.operator=(b); // no implementation on MSVC/STL; calls memcpy?
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
        static_assert(!std::is_move_assignable_v<S>);
        yk::rvariant<S> a;
        static_assert(std::is_move_constructible_v<yk::rvariant<S>>);
        REQUIRE_THROWS(yk::rvariant<S>(std::move(a)));
    }

    // TODO: valueless case
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

TEST_CASE("raw_get")
{
    yk::rvariant<int, float> var = 42;
    using Storage = yk::detail::variant_storage_for<int, float>;
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::raw_get<0>(std::declval<Storage&>())), yk::detail::alternative<0, int>&>);
    STATIC_REQUIRE(std::is_same_v<decltype(yk::detail::raw_get<0>(std::declval<Storage&&>())), yk::detail::alternative<0, int>&&>);
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

TEST_CASE("recursive_wrapper", "[recursive]")
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

template<class... Ts>
constexpr auto FUN_not_UB = []<class T>(T&& t) constexpr {
    yk::detail::FUN<T, yk::rvariant<Ts...>>{}(std::forward<T>(t));
    return true;
};

TEST_CASE("non_recursive_same_as_std") // not [recursive]
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

TEST_CASE("recursive_sentinel", "[recursive]")
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
