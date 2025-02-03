#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <variant>

#include <yk/rvariant.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("helper class") {
  static_assert(yk::variant_size_v<yk::rvariant<int>> == 1);
  static_assert(yk::variant_size_v<yk::rvariant<int, float>> == 2);

  static_assert(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int>>, int>);
  static_assert(std::is_same_v<yk::variant_alternative_t<0, yk::rvariant<int, float>>, int>);
  static_assert(std::is_same_v<yk::variant_alternative_t<1, yk::rvariant<int, float>>, float>);
}

TEST_CASE("type traits") {
  static_assert(std::is_default_constructible_v<yk::rvariant<int, float>>);
  static_assert(std::is_trivially_destructible_v<yk::rvariant<int, float>>);

  static_assert(!std::is_trivially_destructible_v<std::vector<int>>);
  static_assert(!std::is_trivially_destructible_v<yk::rvariant<std::vector<int>>>);
}

TEST_CASE("constructor") {
  {
    yk::rvariant<int, float> v_default;
    yk::rvariant<int, float> v_first(std::in_place_index<0>, 42);
    yk::rvariant<int, float> v_second(std::in_place_index<1>, 3.14f);

    yk::rvariant<std::vector<int>> v_vec(std::in_place_index<0>, {3, 1, 4});
  }

  {
    yk::rvariant<int, float> v_first = 42;
    REQUIRE(v_first.index() == 0);
    yk::rvariant<int, float> v_second = 3.14f;
    REQUIRE(v_second.index() == 1);
  }
}

TEST_CASE("get") {
  yk::rvariant<int, float> v(std::in_place_index<0>, 42);

  int& x = yk::get<0>(v);
  int&& y = yk::get<0>(std::move(v));
}

template <class... Fs>
struct overloaded : Fs... {
  using Fs::operator()...;
};

TEST_CASE("visit") {
  yk::rvariant<int, float, std::string> v(std::in_place_index<2>, "meow");

  const auto vis = overloaded{
      [](int i) { return 0; },
      [](float f) { return 1; },
      [](std::string s) { return 2; },
  };

  REQUIRE(visit(vis, v) == 2);
}

TEST_CASE("recursive") {
  {
    using V = yk::rvariant<int, float, std::vector<yk::recursive_self>>;
    V v1(std::in_place_index<0>, 42);
    V v2(std::in_place_index<1>, 3.14f);
    std::vector<V> vec{v1, v2};
    V v3(std::in_place_index<2>, vec);
  }

  {
    using V = yk::rvariant<int, float, std::vector<yk::recursive_self>>;
    V v1 = 42;
    V v2 = 3.14f;
    std::vector<V> vec{v1, v2};
    V v3 = vec;
  }

  {
    using V = yk::rvariant<int, std::optional<std::vector<yk::recursive_self>>>;
    V v(std::in_place_index<1>, std::vector{V(std::in_place_index<0>, 42)});
  }

  {
    struct UnaryExpr;
    struct BinaryExpr;

    using Expr = yk::rvariant<int, float, std::reference_wrapper<UnaryExpr>, std::reference_wrapper<BinaryExpr>>;

    struct UnaryExpr {
      int op;
      Expr expr;
    };

    struct BinaryExpr {
      Expr lhs;
      int op;
      Expr rhs;
    };
  }
}
