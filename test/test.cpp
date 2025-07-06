#include <type_traits>
#include <utility>

#include <yk/rvariant.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("pack indexing") {
  STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int>, int>);
  STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int, float>, int>);
  STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<1, int, float>, float>);
}

TEST_CASE("helper class") {
  STATIC_REQUIRE(yk::variant_size_v<yk::rvariant<int>> == 1);
  STATIC_REQUIRE(yk::variant_size_v<yk::rvariant<int, float>> == 2);

  STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int>>, int>);
  STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int, float>>, int>);
  STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<1, yk::rvariant<int, float>>, float>);

  STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, const yk::rvariant<int>>, const int>);
  STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<0, const yk::rvariant<int, float>>, const int>);
  STATIC_REQUIRE(std::is_same_v<yk::variant_alternative_t<1, const yk::rvariant<int, float>>, const float>);
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
      S(const S&) {}
    };
    STATIC_REQUIRE_FALSE(std::is_trivially_copy_constructible_v<S>);
    STATIC_REQUIRE_FALSE(std::is_trivially_copy_constructible_v<yk::rvariant<S>>);
    STATIC_REQUIRE_FALSE(std::is_trivially_copy_constructible_v<yk::rvariant<int, S>>);
  }
  {
    struct S {
      S(const S&) = delete;
    };
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<S>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<yk::rvariant<S>>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<yk::rvariant<int, S>>);
  }
  {
    struct S {
      S() {}
      S(const S&) { throw std::exception(); }
    };
    yk::rvariant<S> a;
    REQUIRE_THROWS(yk::rvariant<S>(a));
  }
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
}
