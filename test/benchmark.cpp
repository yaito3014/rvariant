// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/rvariant.hpp>

#include <yk/default_init_allocator.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <print>
#include <chrono>
#include <vector>
#include <variant>
#include <random>

namespace unit_test {

namespace {

template<std::size_t I>
struct int_identity
{
    using type = int;
};

template<template<class...> class VTT, std::size_t AltN, template<std::size_t> class TTT, class Seq = std::make_index_sequence<AltN>>
struct many_V_impl;

template<std::size_t AltN, template<class...> class VTT, template<std::size_t> class TTT, std::size_t... Is>
struct many_V_impl<VTT, AltN, TTT, std::index_sequence<Is...>>
{
    using type = VTT<typename TTT<Is>::type...>;
};

template<template<class...> class VTT, std::size_t AltN, template<std::size_t> class TTT>
using many_V_t = typename many_V_impl<VTT, AltN, TTT>::type;

} // anonymous

TEST_CASE("benchmark")
{
    using Clock = std::chrono::high_resolution_clock;
    //static_assert(Clock::is_steady);
    using duration_type = std::chrono::duration<double, std::milli>;

    constexpr std::size_t N = 100'0000;
    constexpr std::size_t AltN = 16;

    using REng = std::mt19937;

    std::println("N: {}, alternatives: {}", N, AltN);
    std::println();

    auto const do_bench = [&]<class V>(std::string_view const bench_name)
    {
        std::random_device rd;

        std::uniform_int_distribution<std::size_t> I_dist(0, AltN - 1);
        REng I_eng(rd());

        std::uniform_int_distribution<int> value_dist;
        REng value_eng(rd());

        std::vector<V, yk::default_init_allocator<V>> vars;
        vars.reserve(N);

        {
            auto const start_time = Clock::now();
            for (std::size_t i = 0; i < N; ++i) {
                auto const value = value_dist(value_eng);

                switch (I_dist(I_eng)) {
                case  0: vars.emplace_back(std::in_place_index< 0>, value); break;
                case  1: vars.emplace_back(std::in_place_index< 1>, value); break;
                case  2: vars.emplace_back(std::in_place_index< 2>, value); break;
                case  3: vars.emplace_back(std::in_place_index< 3>, value); break;
                case  4: vars.emplace_back(std::in_place_index< 4>, value); break;
                case  5: vars.emplace_back(std::in_place_index< 5>, value); break;
                case  6: vars.emplace_back(std::in_place_index< 6>, value); break;
                case  7: vars.emplace_back(std::in_place_index< 7>, value); break;
                case  8: vars.emplace_back(std::in_place_index< 8>, value); break;
                case  9: vars.emplace_back(std::in_place_index< 9>, value); break;
                case 10: vars.emplace_back(std::in_place_index<10>, value); break;
                case 11: vars.emplace_back(std::in_place_index<11>, value); break;
                case 12: vars.emplace_back(std::in_place_index<12>, value); break;
                case 13: vars.emplace_back(std::in_place_index<13>, value); break;
                case 14: vars.emplace_back(std::in_place_index<14>, value); break;
                case 15: vars.emplace_back(std::in_place_index<15>, value); break;
                default: std::unreachable();
                }
            }
            auto const end_time = Clock::now();
            auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
            std::println("[{}] construction: {} ms", bench_name, elapsed.count());
        }

#if YK_RVARIANT_VISIT_STRENGTHEN
        constexpr bool is_std = yk::core::is_ttp_specialization_of_v<V, std::variant>;
        static_assert(is_std || noexcept(visit([](int const&) noexcept(true) {}, vars[0])));
        static_assert(is_std || !noexcept(visit([](int const&) noexcept(false) {}, vars[0])));
#endif

        {
            unsigned long long sum = 0;

            auto const start_time = Clock::now();
            for (std::size_t i = 0; i < N; ++i) {
                visit([&](int const& value) noexcept {
                    sum += value;
                }, vars[i]);
            }
            auto const end_time = Clock::now();
            auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
            std::println("[{}] visit: {} ms", bench_name, elapsed.count());

            std::println("computation result: {}", sum);
        }

        std::println();
    };

    do_bench.operator()<many_V_t<std::variant, AltN, int_identity>>("std::variant");
    do_bench.operator()<many_V_t<yk::rvariant, AltN, int_identity>>("yk::rvariant");
}

} // unit_test
