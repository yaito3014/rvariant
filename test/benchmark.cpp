// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "benchmark_support.hpp"

#include <yk/rvariant/rvariant.hpp>

#include <yk/default_init_allocator.hpp>

#include <charconv>
#include <string_view>
#include <print>
#include <chrono>
#include <vector>
#include <variant>
#include <random>

#include <cstdlib>

namespace benchmark {

namespace {

template<template<class...> class VTT, std::size_t AltN, class T, class Seq = std::make_index_sequence<AltN>>
struct many_V_impl;

template<std::size_t AltN, template<class...> class VTT, class T, std::size_t... Is>
struct many_V_impl<VTT, AltN, T, std::index_sequence<Is...>>
{
    template<auto>
    struct type_identity
    {
        using type = T;
    };

    using type = VTT<typename type_identity<Is>::type...>;
};

template<template<class...> class VTT, std::size_t AltN, class T>
using many_V_t = typename many_V_impl<VTT, AltN, T>::type;


using Clock = std::chrono::high_resolution_clock;
//static_assert(Clock::is_steady);
using duration_type = std::chrono::duration<double, std::milli>;

inline constexpr std::size_t AltN = 16;

using REng = std::mt19937;

template<class Vars>
void benchmark_construct(std::size_t const N, Vars& vars)
{
    std::random_device rd;

    std::uniform_int_distribution<std::size_t> I_dist(0, AltN - 1);
    REng I_eng(rd());

    std::uniform_int_distribution<int> value_dist;
    REng value_eng(rd());

    vars.reserve(N);

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
    std::println("construction: {:.3f} ms", elapsed.count());
}

template<class Vars>
void benchmark_visit(std::size_t const N, Vars const& vars)
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
    std::println("visit: {:.3f} ms", elapsed.count());

    disable_optimization(sum);
}

template<class Vars>
void benchmark_multi_visit(std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 1; i < N; ++i) {
        visit([&](int const& a, int const& b) noexcept {
            sum += a + b;
        }, vars[i], vars[i - 1]);
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    std::println("multi visit ({}^2): {:.3f} ms", AltN, elapsed.count());

    disable_optimization(sum);
}

template<class Vars>
void benchmark_get(std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        switch (vars[i].index()) {
        case  0: sum += get< 0>(vars[i]); break;
        case  1: sum += get< 1>(vars[i]); break;
        case  2: sum += get< 2>(vars[i]); break;
        case  3: sum += get< 3>(vars[i]); break;
        case  4: sum += get< 4>(vars[i]); break;
        case  5: sum += get< 5>(vars[i]); break;
        case  6: sum += get< 6>(vars[i]); break;
        case  7: sum += get< 7>(vars[i]); break;
        case  8: sum += get< 8>(vars[i]); break;
        case  9: sum += get< 9>(vars[i]); break;
        case 10: sum += get<10>(vars[i]); break;
        case 11: sum += get<11>(vars[i]); break;
        case 12: sum += get<12>(vars[i]); break;
        case 13: sum += get<13>(vars[i]); break;
        case 14: sum += get<14>(vars[i]); break;
        case 15: sum += get<15>(vars[i]); break;
        default: std::unreachable();
        }
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    std::println("get: {:.3f} ms", elapsed.count());

    disable_optimization(sum);
}

template<class Vars>
void benchmark_get_if(std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        if (auto* ptr = get_if< 0>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 1>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 2>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 3>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 4>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 5>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 6>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 7>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 8>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if< 9>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if<10>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if<11>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if<12>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if<13>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if<14>(&vars[i])) { sum += *ptr; continue; }
        if (auto* ptr = get_if<15>(&vars[i])) { sum += *ptr; continue; }
        //std::unreachable(); // makes the entire benchmark slower
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    std::println("get_if: {:.3f} ms", elapsed.count());

    disable_optimization(sum);
}

template<class Comp, class Vars>
void benchmark_operator(std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 1; i < N; ++i) {
        sum += Comp{}(vars[i], vars[i - 1]) == 0;
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    std::println(": {:.3f} ms", elapsed.count());

    disable_optimization(sum);
}

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

    std::random_device rd;

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
    std::println("");

    // ----------------------------------------------------------

    {
        using V = many_V_t<std::variant, AltN, int>;
        std::println("[std::variant]");

        std::vector<V, yk::default_init_allocator<V>> vars;
        benchmark_construct(N, vars);

        benchmark_get(N, vars);
        benchmark_get_if(N, vars);
        benchmark_visit(N, vars);
        benchmark_multi_visit(N, vars);

        std::print("operator== "); benchmark_operator<std::equal_to<>>(N, vars);
        std::print("operator!= "); benchmark_operator<std::not_equal_to<>>(N, vars);
        std::print("operator<  "); benchmark_operator<std::less<>>(N, vars);
        std::print("operator>  "); benchmark_operator<std::greater<>>(N, vars);
        std::print("operator<= "); benchmark_operator<std::less_equal<>>(N, vars);
        std::print("operator>= "); benchmark_operator<std::greater_equal<>>(N, vars);
        std::print("operator<=>"); benchmark_operator<std::compare_three_way>(N, vars);

        std::println("");
    }
    {
        using V = many_V_t<yk::rvariant, AltN, int>;
        std::println("[yk::rvariant]");

        std::vector<V, yk::default_init_allocator<V>> vars;
        benchmark_construct(N, vars);

        benchmark_get(N, vars);
        benchmark_get_if(N, vars);
        benchmark_visit(N, vars);
        benchmark_multi_visit(N, vars);

        std::print("operator== "); benchmark_operator<std::equal_to<>>(N, vars);
        std::print("operator!= "); benchmark_operator<std::not_equal_to<>>(N, vars);
        std::print("operator<  "); benchmark_operator<std::less<>>(N, vars);
        std::print("operator>  "); benchmark_operator<std::greater<>>(N, vars);
        std::print("operator<= "); benchmark_operator<std::less_equal<>>(N, vars);
        std::print("operator>= "); benchmark_operator<std::greater_equal<>>(N, vars);
        std::print("operator<=>"); benchmark_operator<std::compare_three_way>(N, vars);

        std::println("");
    }

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
