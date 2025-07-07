#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <yk/detail/exactly_once.hpp>
#include <yk/detail/is_unique.hpp>
#include <yk/detail/pack_indexing.hpp>
#include <yk/rvariant.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("pack indexing") {
  STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int>, int>);
  STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<0, int, float>, int>);
  STATIC_REQUIRE(std::is_same_v<yk::detail::pack_indexing_t<1, int, float>, float>);
}

TEST_CASE("exactly once") {
  STATIC_REQUIRE(yk::detail::exactly_once_v<int, int, float>);
  STATIC_REQUIRE_FALSE(yk::detail::exactly_once_v<int, int, int>);
}

TEST_CASE("is in") {
  STATIC_REQUIRE(yk::detail::is_in_v<int, int, float>);
  STATIC_REQUIRE_FALSE(yk::detail::is_in_v<int, float>);
}

TEST_CASE("is unique") {
  STATIC_REQUIRE(yk::detail::is_unique_v<int, float>);
  STATIC_REQUIRE_FALSE(yk::detail::is_unique_v<int, int>);
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
      S(const S&) {}
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
