#include "yk/core/type_traits.hpp"

#include <catch2/catch_test_macros.hpp>

namespace unit_test {

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

} // unit_test
