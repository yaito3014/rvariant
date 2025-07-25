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


TEST_CASE("pack_indexing", "[core]")
{
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<0, int>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<0, int, float>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<1, int, float>, float>);
}

TEST_CASE("exactly_once", "[core]")
{
    STATIC_REQUIRE(yk::core::exactly_once_v<int, yk::core::type_list<int, float>>);
    STATIC_REQUIRE_FALSE(yk::core::exactly_once_v<int, yk::core::type_list<int, int>>);
}

TEST_CASE("is_in", "[core]")
{
    STATIC_REQUIRE(yk::core::is_in_v<int, int, float>);
    STATIC_REQUIRE_FALSE(yk::core::is_in_v<int, float>);
}

TEST_CASE("find_index", "[core]")
{
    STATIC_REQUIRE(yk::core::find_index_v<int,    yk::core::type_list<int, float, double>> == 0);
    STATIC_REQUIRE(yk::core::find_index_v<float,  yk::core::type_list<int, float, double>> == 1);
    STATIC_REQUIRE(yk::core::find_index_v<double, yk::core::type_list<int, float, double>> == 2);
    STATIC_REQUIRE(yk::core::find_index_v<int,    yk::core::type_list<float, double>> == yk::core::find_npos);
}

TEST_CASE("pack_union", "[core]")
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

struct S
{
    S() { throw std::exception{}; }
    S(S const&) = default;
    S(S&&) = default;
    S& operator=(S const&) = default;
    S& operator=(S&&) = default;
    ~S() = default;
};
static_assert(!std::is_nothrow_default_constructible_v<S>);
static_assert(!std::is_nothrow_default_constructible_v<yk::rvariant<S>>);

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
}

TEST_CASE("raw_get")
{
    using yk::detail::raw_get;
    using Storage = yk::detail::make_variadic_union_t<int>;

    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage&>())), int&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage const&>())), int const&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage&&>())), int&&>);
    STATIC_REQUIRE(std::is_same_v<decltype(raw_get<0>(std::declval<Storage const&&>())), int const&&>);
}

template<bool IsNoexcept, class R, class F, class Visitor, class Storage>
constexpr bool is_noexcept_invocable_r_v = std::conditional_t<
    IsNoexcept,
    std::conjunction<std::is_invocable_r<R, F, Visitor, Storage>, std::is_nothrow_invocable_r<R, F, Visitor, Storage>>,
    std::conjunction<std::is_invocable_r<R, F, Visitor, Storage>, std::negation<std::is_nothrow_invocable_r<R, F, Visitor, Storage>>>
>::value;

TEST_CASE("raw_visit")
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


// generate (bool, bool, ...) for testing

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

template<bool AllNeverValueless, auto... n>
using never_valueless_seq = typename detail::never_valueless_seq_impl<AllNeverValueless, std::integer_sequence<bool>, n...>::type;

template<auto... n>
using nv_true_flat_index = yk::detail::flat_index<std::index_sequence<n...>, never_valueless_seq<true, n...>>;

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

template<class T>
struct strong
{
    T value;
};

TEST_CASE("visit")
{
    // TODO: valueless case

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

template<std::size_t I>
struct just_index {};

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

    // TODO: valueless case
}

TEST_CASE("holds_alternative")
{
    {
        yk::rvariant<int, float> var = 42;
        REQUIRE(yk::holds_alternative<int>(var));
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
    // TODO: valueless case
}
