// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "yk/core/type_traits.hpp"

#include <catch2/catch_test_macros.hpp>

namespace unit_test {

TEST_CASE("pack_indexing", "[core]")
{
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<0, int>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<0, int, float>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<1, int, float>, float>);

    // make sure non-object and non-referenceable type is working
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<0, void>, void>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<0, void, int>, void>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<1, void, int>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<0, int, void>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<1, int, void>, void>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<0, void, void>, void>);
    STATIC_REQUIRE(std::is_same_v<yk::core::pack_indexing_t<1, void, void>, void>);

    STATIC_REQUIRE(std::is_same_v<yk::core::at_c_t<0, yk::core::type_list<int>>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::core::at_c_t<0, yk::core::type_list<int, float>>, int>);
    STATIC_REQUIRE(std::is_same_v<yk::core::at_c_t<1, yk::core::type_list<int, float>>, float>);
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

    STATIC_REQUIRE(yk::core::find_index_v<int, yk::core::type_list<int, int, double>> == 0);
}

TEST_CASE("Cpp17EqualityComparable", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17EqualityComparable<int>);
    {
        struct S
        {
            bool operator==(S const&) const { return true; }
        };
        STATIC_REQUIRE(yk::core::Cpp17EqualityComparable<S>);
    }
    {
        struct S
        {
            bool operator==(S const&) const = delete;
        };
        STATIC_REQUIRE(!yk::core::Cpp17EqualityComparable<S>);
    }
}

TEST_CASE("Cpp17LessThanComparable", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17LessThanComparable<int>);
    {
        struct S
        {
            bool operator<(S const&) const { return false; }
        };
        STATIC_REQUIRE(yk::core::Cpp17LessThanComparable<S>);
    }
    {
        struct S
        {
            bool operator<(S const&) const = delete;
        };
        STATIC_REQUIRE(!yk::core::Cpp17LessThanComparable<S>);
    }
}

TEST_CASE("Cpp17DefaultConstructible", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17DefaultConstructible<int>);
    {
        using T = int const;
        STATIC_REQUIRE(std::is_default_constructible_v<T>);
        // ReSharper disable once CppStaticAssertFailure
        STATIC_REQUIRE(!std::default_initializable<T>);
        // ReSharper disable once CppStaticAssertFailure
        STATIC_REQUIRE(!yk::core::Cpp17DefaultConstructible<T>);
    }
    {
        using T = int const[10];
        STATIC_REQUIRE(std::is_default_constructible_v<T>);
        STATIC_REQUIRE(!yk::core::Cpp17DefaultConstructible<T>);
    }
    {
        struct S
        {
            S() = delete;
        };
        STATIC_REQUIRE(!std::is_default_constructible_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17DefaultConstructible<S>);
    }
}

TEST_CASE("Cpp17MoveConstructible", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17MoveConstructible<int>);
    {
        using T = int const;
        STATIC_REQUIRE(std::is_move_constructible_v<T>);
        STATIC_REQUIRE(yk::core::Cpp17MoveConstructible<T>);
    }
    {
        using T = int const[10];
        STATIC_REQUIRE(!std::is_move_constructible_v<T>);
        STATIC_REQUIRE(!std::move_constructible<T>);
        STATIC_REQUIRE(!yk::core::Cpp17MoveConstructible<T>);
    }
    {
        struct S
        {
            S(S&&) = delete;
        };
        STATIC_REQUIRE(!std::is_move_constructible_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17MoveConstructible<S>);
    }
    {
        struct S
        {
            explicit S(S&&) noexcept {}
        };
        STATIC_REQUIRE(std::is_move_constructible_v<S>);
        STATIC_REQUIRE(!std::move_constructible<S>);
        STATIC_REQUIRE(!yk::core::Cpp17MoveConstructible<S>);
    }
}

TEST_CASE("Cpp17CopyConstructible", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17CopyConstructible<int>);
    {
        using T = int const;
        STATIC_REQUIRE(std::is_copy_constructible_v<T>);
        STATIC_REQUIRE(yk::core::Cpp17CopyConstructible<T>);
    }
    {
        using T = int const[10];
        STATIC_REQUIRE(!std::is_copy_constructible_v<T>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyConstructible<T>);
    }
    {
        struct S
        {
            int const t[10];  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        };
        STATIC_REQUIRE(std::is_copy_constructible_v<S>);
        STATIC_REQUIRE(yk::core::Cpp17CopyConstructible<S>);
    }
    {
        struct S
        {
            S(S&&) = default;
            S(S const&) = delete;
        };
        STATIC_REQUIRE(!std::is_copy_constructible_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyConstructible<S>);
    }
    {
        struct S
        {
            S(S&&) = delete;
            S(S const&) = default;
        };
        STATIC_REQUIRE(std::is_copy_constructible_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyConstructible<S>);
    }
    {
        struct S
        {
            explicit S(S const&) {}  // NOLINT(modernize-use-equals-default)
        };
        STATIC_REQUIRE(std::is_copy_constructible_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyConstructible<S>);
    }
    {
        struct S
        {
            S(S&) = delete;
            S(S const&) {}  // NOLINT(modernize-use-equals-default)
            S(S const&&) noexcept {}
        };
        STATIC_REQUIRE(std::is_copy_constructible_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyConstructible<S>);
    }
    {
        struct S
        {
            S(S&) {}  // NOLINT(modernize-use-equals-default)
            S(S const&) = delete;
            S(S const&&) noexcept {}
        };
        STATIC_REQUIRE(!std::is_copy_constructible_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyConstructible<S>);
    }
    {
        struct S
        {
            S(S&) {}  // NOLINT(modernize-use-equals-default)
            S(S const&) {}  // NOLINT(modernize-use-equals-default)
            S(S const&&) = delete;
        };
        STATIC_REQUIRE(std::is_copy_constructible_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyConstructible<S>);
    }
}

TEST_CASE("Cpp17MoveAssignable", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17MoveAssignable<int>);
    {
        using T = int const;
        STATIC_REQUIRE(!std::is_move_assignable_v<T>);
        STATIC_REQUIRE(!std::assignable_from<T&, T&&>);
        STATIC_REQUIRE(!yk::core::Cpp17MoveAssignable<T>);
    }
    {
        using T = int const [10];
        STATIC_REQUIRE(!std::is_move_assignable_v<T>);
        STATIC_REQUIRE(!std::assignable_from<T&, T&&>);
        STATIC_REQUIRE(!yk::core::Cpp17MoveAssignable<T>);
    }
    {
        struct S
        {
            S& operator=(S&&) = delete;
        };
        STATIC_REQUIRE(!std::is_move_assignable_v<S>);
        STATIC_REQUIRE(!std::assignable_from<S&, S&&>);
        STATIC_REQUIRE(!yk::core::Cpp17MoveAssignable<S>);
    }
}

TEST_CASE("Cpp17CopyAssignable", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17CopyAssignable<int>);
    {
        using T = int const;
        STATIC_REQUIRE(!std::is_copy_assignable_v<T>);
        STATIC_REQUIRE(!std::assignable_from<T&, T const&>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyAssignable<T>);
    }
    {
        using T = int const [10];
        STATIC_REQUIRE(!std::is_copy_assignable_v<T>);
        STATIC_REQUIRE(!std::assignable_from<T&, T const&>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyAssignable<T>);
    }
    {
        struct S
        {
            S& operator=(S const&) = delete;
        };
        STATIC_REQUIRE(!std::is_copy_assignable_v<S>);
        STATIC_REQUIRE(!std::assignable_from<S&, S const&>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyAssignable<S>);
    }
    {
        struct S
        {
            S& operator=(S&) = delete;
            S& operator=(S const&) { return *this; }  // NOLINT(modernize-use-equals-default)
            S& operator=(S const&&) noexcept { return *this; }  // NOLINT(misc-unconventional-assign-operator)
        };
        STATIC_REQUIRE(std::is_copy_assignable_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyAssignable<S>);
    }
    {
        struct S
        {
            S& operator=(S&) { return *this; }  // NOLINT(modernize-use-equals-default, misc-unconventional-assign-operator)
            S& operator=(S const&) = delete;
            S& operator=(S const&&) noexcept { return *this; }  // NOLINT(misc-unconventional-assign-operator)
        };
        STATIC_REQUIRE(!std::is_copy_assignable_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyAssignable<S>);
    }
    {
        struct S
        {
            S& operator=(S&) { return *this; }  // NOLINT(misc-unconventional-assign-operator, modernize-use-equals-default)
            S& operator=(S const&) { return *this; }  // NOLINT(modernize-use-equals-default)
            S& operator=(S const&&) = delete;
        };
        STATIC_REQUIRE(std::is_copy_assignable_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyAssignable<S>);
    }
    {
        struct S
        {
            S& operator=(S const&) { return *this; }  // NOLINT(modernize-use-equals-default)
            S& operator=(S&&) = delete;
        };
        STATIC_REQUIRE(std::is_copy_assignable_v<S>);
        STATIC_REQUIRE(!yk::core::Cpp17CopyAssignable<S>);
    }
}

TEST_CASE("Cpp17Destructible", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17Destructible<int>);
    {
        struct S
        {
            ~S() {}  // NOLINT(modernize-use-equals-default)
        };
        STATIC_REQUIRE(std::is_nothrow_destructible_v<S>);
        STATIC_REQUIRE(yk::core::Cpp17Destructible<S>);
    }
    {
        struct S
        {
            ~S() noexcept(true) {}  // NOLINT(modernize-use-equals-default)
        };
        STATIC_REQUIRE(std::is_nothrow_destructible_v<S>);
        STATIC_REQUIRE(yk::core::Cpp17Destructible<S>);
    }
    {
        struct S
        {
            ~S() noexcept(false) {}  // NOLINT(modernize-use-equals-default)
        };
        STATIC_REQUIRE(std::is_destructible_v<S>);
        STATIC_REQUIRE(!std::is_nothrow_destructible_v<S>);
        STATIC_REQUIRE(yk::core::Cpp17Destructible<S>); // noexcept(false) should still satisfy this, as long as the exception is not propagated

        // Cpp17Destructible is NOT the same as std::destructible
        STATIC_REQUIRE(!std::destructible<S>); // noexcept(false) is not satisfied
    }
    {
        // https://eel.is/c++draft/utility.arg.requirements#tab:cpp17.destructible-row-3-column-1-note-3
        // > Array types and non-object types are not Cpp17Destructible.

        STATIC_REQUIRE(!yk::core::Cpp17Destructible<int[]>);
        STATIC_REQUIRE(!yk::core::Cpp17Destructible<int[1]>);
        STATIC_REQUIRE(!yk::core::Cpp17Destructible<void>);
        STATIC_REQUIRE(!yk::core::Cpp17Destructible<int&>);

        // Cpp17Destructible is NOT the same as std::destructible
        STATIC_REQUIRE(!std::destructible<int[]>);
        STATIC_REQUIRE( std::destructible<int[1]>);
        STATIC_REQUIRE(!std::destructible<void>);
        STATIC_REQUIRE( std::destructible<int&>);
    }
}

namespace {

struct NonMovable
{
    NonMovable() {}  // NOLINT(modernize-use-equals-default)
    NonMovable(NonMovable const&) = delete;
    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable const&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
    ~NonMovable() {}  // NOLINT(modernize-use-equals-default)
};

struct S
{
    [[maybe_unused]] NonMovable nm;
};

struct Ss
{
    [[maybe_unused]] NonMovable nm;
    friend void swap(Ss&, Ss&) noexcept {}
};

} // anonymous

TEST_CASE("Cpp17Swappable", "[core]")
{
    STATIC_REQUIRE(yk::core::Cpp17Swappable<int>);
    {
        STATIC_REQUIRE(!std::is_swappable_v<S&>);
        STATIC_REQUIRE(!yk::core::Cpp17Swappable<S>);
    }
    {
        STATIC_REQUIRE(std::is_swappable_v<Ss&>);
        STATIC_REQUIRE(yk::core::Cpp17Swappable<Ss>);

        Ss a, b;
        using std::swap;
        swap(a, b);
    }
}

} // unit_test
