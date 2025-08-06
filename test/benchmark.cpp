// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "benchmark_support.hpp"

#include <yk/rvariant/rvariant.hpp>

#include <yk/default_init_allocator.hpp>

#include <string_view>
#include <print>
#include <chrono>
#include <vector>
#include <variant>
#include <random>

#include <cstdlib>

namespace benchmark {

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


int benchmark_main(std::size_t const N)
{
#ifndef NDEBUG
    std::println(
        "\n"
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
        "   WARNING: This is NOT a Release build\n"
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
    );
#endif

    using Clock = std::chrono::high_resolution_clock;
    //static_assert(Clock::is_steady);
    using duration_type = std::chrono::duration<double, std::milli>;

    constexpr std::size_t AltN = 16;

    std::random_device rd;
    using REng = std::mt19937;

    // ----------------------------------------------------------

    std::println("N: {}, alternatives: {}", N, AltN);
    {
        unsigned long long sum = 0;

        auto const start_time = Clock::now();
        for (std::size_t i = 1; i < N; ++i) {
            sum += N % i;
        }
        auto const end_time = Clock::now();
        auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
        std::println("Adding loop-based int N times: {:.3f} ms", elapsed.count());

        disable_optimization(sum);
    }
    {
        std::uniform_int_distribution<int> value_dist;
        REng value_eng(rd());

        unsigned long long sum = 0;

        auto const start_time = Clock::now();
        for (std::size_t i = 0; i < N; ++i) {
            int const value = value_dist(value_eng);
            sum += value;
        }
        auto const end_time = Clock::now();
        auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
        std::println("Adding random int N times: {:.3f} ms", elapsed.count());

        disable_optimization(sum);
    }
    std::println();

    // ----------------------------------------------------------

    auto const do_bench = [&]<class V>(std::string_view const bench_name)
    {
        std::uniform_int_distribution<std::size_t> I_dist(0, AltN - 1);
        REng I_eng(rd());

        std::uniform_int_distribution<int> value_dist;
        REng value_eng(rd());

        std::vector<V, yk::default_init_allocator<V>> vars;
        vars.reserve(N);

        {
            auto const start_time = Clock::now();
            for (std::size_t i = 0; i < N; ++i) {
                int const value = value_dist(value_eng);

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
            std::println("[{}] construction: {:.3f} ms", bench_name, elapsed.count());
        }

#if YK_RVARIANT_VISIT_STRENGTHEN
        constexpr bool is_std = yk::core::is_ttp_specialization_of_v<V, std::variant>;
        static_assert(is_std || noexcept(visit([](int const&) noexcept(true) {}, vars[0])));
        static_assert(is_std || !noexcept(visit([](int const&) noexcept(false) {}, vars[0])));
#endif

        // check visit performance
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
            std::println("[{}] visit: {:.3f} ms", bench_name, elapsed.count());

            disable_optimization(sum);
        }

        // check raw_visit performance
        {
            unsigned long long sum = 0;

            auto const start_time = Clock::now();
            for (std::size_t i = 1; i < N; ++i) {
                sum += vars[i] < vars[i - 1];
            }
            auto const end_time = Clock::now();
            auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
            std::println("[{}] operator<: {:.3f} ms", bench_name, elapsed.count());

            disable_optimization(sum);
        }

        std::println();
    };

    do_bench.operator()<many_V_t<std::variant, AltN, int_identity>>("std::variant");
    do_bench.operator()<many_V_t<yk::rvariant, AltN, int_identity>>("yk::rvariant");

    return EXIT_SUCCESS;
}

} // anonymous

} // benchmark

int main(int argc, char* argv[])
{
    if (argc <= 1) {
        std::println("usage: ./yk_rvariant_benchmark [N]");
        return EXIT_FAILURE;
    }
    std::string_view const N_str(argv[1]);

    unsigned long long N = 1;
    auto const [ptr, ec] = std::from_chars(N_str.data(), N_str.data() + N_str.size(), N);
    if (ec != std::errc{}) {
        std::println("parse error");
        return EXIT_FAILURE;
    }

    return benchmark::benchmark_main(N);
}
