// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "benchmark_support.hpp"

#include <yk/rvariant/rvariant.hpp>

#include <yk/default_init_allocator.hpp>

#include <fstream>
#include <ranges>
#include <utility>
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


template<class T>
T make_value(int rand)
{
    if constexpr (std::is_same_v<T, std::string>) {
        return std::to_string(rand);
    } else {
        return rand;
    }
}

template<class T>
int read_value(T const& val)
{
    if constexpr (std::is_same_v<T, std::string>) {
        return static_cast<int>(val.size());
    } else {
        return val;
    }
}


using Clock = std::chrono::high_resolution_clock;
//static_assert(Clock::is_steady);
using duration_type = std::chrono::duration<double, std::milli>;

using REng = std::mt19937;

struct Table
{
    explicit Table(std::string type_name)
        : type_name(std::move(type_name))
    {}

    std::string type_name;
    std::size_t AltN{};
    std::size_t N{};

    struct Entry
    {
        std::string key;
        duration_type duration;
    };
    using EntryList = std::vector<Entry>;
    EntryList std_datas, rva_datas;

    std::string make_csv() const
    {
        std::string csv;
        csv += std::format("T={} | alternatives={} | N={},std::variant,rvariant\n", type_name, AltN, N);

        for (auto const& [a, b] : std::views::zip(std_datas, rva_datas)) {
            if (a.key != b.key) throw std::logic_error{"invalid scheme"};
            csv += std::format("{},{},{}\n", a.key, a.duration.count(), b.duration.count());
        }
        return csv;
    }
};

template<class T, class Vars>
void benchmark_construct_3(Table::EntryList& entries, std::size_t const N, Vars& vars)
{
    std::random_device rd;

    std::uniform_int_distribution<std::size_t> I_dist(0, 3 - 1);
    REng I_eng(rd());

    std::uniform_int_distribution<int> value_dist;
    REng value_eng(rd());

    vars.reserve(N);

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        auto value = make_value<T>(value_dist(value_eng));

        switch (I_dist(I_eng)) {
        case  0: vars.emplace_back(std::in_place_index< 0>, std::move(value)); break;
        case  1: vars.emplace_back(std::in_place_index< 1>, std::move(value)); break;
        case  2: vars.emplace_back(std::in_place_index< 2>, std::move(value)); break;
        default: std::unreachable();
        }
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("construction", elapsed);
}

template<class T, class Vars>
void benchmark_construct_16(Table::EntryList& entries, std::size_t const N, Vars& vars)
{
    std::random_device rd;

    std::uniform_int_distribution<std::size_t> I_dist(0, 16 - 1);
    REng I_eng(rd());

    std::uniform_int_distribution<int> value_dist;
    REng value_eng(rd());

    vars.reserve(N);

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        auto value = make_value<T>(value_dist(value_eng));

        switch (I_dist(I_eng)) {
        case  0: vars.emplace_back(std::in_place_index< 0>, std::move(value)); break;
        case  1: vars.emplace_back(std::in_place_index< 1>, std::move(value)); break;
        case  2: vars.emplace_back(std::in_place_index< 2>, std::move(value)); break;
        case  3: vars.emplace_back(std::in_place_index< 3>, std::move(value)); break;
        case  4: vars.emplace_back(std::in_place_index< 4>, std::move(value)); break;
        case  5: vars.emplace_back(std::in_place_index< 5>, std::move(value)); break;
        case  6: vars.emplace_back(std::in_place_index< 6>, std::move(value)); break;
        case  7: vars.emplace_back(std::in_place_index< 7>, std::move(value)); break;
        case  8: vars.emplace_back(std::in_place_index< 8>, std::move(value)); break;
        case  9: vars.emplace_back(std::in_place_index< 9>, std::move(value)); break;
        case 10: vars.emplace_back(std::in_place_index<10>, std::move(value)); break;
        case 11: vars.emplace_back(std::in_place_index<11>, std::move(value)); break;
        case 12: vars.emplace_back(std::in_place_index<12>, std::move(value)); break;
        case 13: vars.emplace_back(std::in_place_index<13>, std::move(value)); break;
        case 14: vars.emplace_back(std::in_place_index<14>, std::move(value)); break;
        case 15: vars.emplace_back(std::in_place_index<15>, std::move(value)); break;
        default: std::unreachable();
        }
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("construction", elapsed);
}

template<class Vars>
YK_FORCEINLINE void benchmark_copy_assign(Table::EntryList& entries, std::size_t const N, Vars& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 1; i < N; ++i) {
        vars[i] = vars[i - 1];
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("copy assign", elapsed);

    disable_optimization(sum);
}

template<class Vars>
void benchmark_visit(Table::EntryList& entries, std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        visit([&](auto const& value) noexcept {
            sum += read_value(value);
        }, vars[i]);
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("visit", elapsed);

    disable_optimization(sum);
}

template<class Vars>
void benchmark_multi_visit(Table::EntryList& entries, std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 1; i < N; ++i) {
        visit([&](auto const& a, auto const& b) noexcept {
            sum += read_value(a) + read_value(b);
        }, vars[i], vars[i - 1]);
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("multi visit (2 vars)", elapsed);

    disable_optimization(sum);
}

template<class Vars>
void benchmark_get_3(Table::EntryList& entries, std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        switch (vars[i].index()) {
        case  0: sum += read_value(get< 0>(vars[i])); break;
        case  1: sum += read_value(get< 1>(vars[i])); break;
        case  2: sum += read_value(get< 2>(vars[i])); break;
        default: std::unreachable();
        }
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("get", elapsed);

    disable_optimization(sum);
}

template<class Vars>
void benchmark_get_16(Table::EntryList& entries, std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        switch (vars[i].index()) {
        case  0: sum += read_value(get< 0>(vars[i])); break;
        case  1: sum += read_value(get< 1>(vars[i])); break;
        case  2: sum += read_value(get< 2>(vars[i])); break;
        case  3: sum += read_value(get< 3>(vars[i])); break;
        case  4: sum += read_value(get< 4>(vars[i])); break;
        case  5: sum += read_value(get< 5>(vars[i])); break;
        case  6: sum += read_value(get< 6>(vars[i])); break;
        case  7: sum += read_value(get< 7>(vars[i])); break;
        case  8: sum += read_value(get< 8>(vars[i])); break;
        case  9: sum += read_value(get< 9>(vars[i])); break;
        case 10: sum += read_value(get<10>(vars[i])); break;
        case 11: sum += read_value(get<11>(vars[i])); break;
        case 12: sum += read_value(get<12>(vars[i])); break;
        case 13: sum += read_value(get<13>(vars[i])); break;
        case 14: sum += read_value(get<14>(vars[i])); break;
        case 15: sum += read_value(get<15>(vars[i])); break;
        default: std::unreachable();
        }
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("get", elapsed);

    disable_optimization(sum);
}

template<class Vars>
void benchmark_get_if_3(Table::EntryList& entries, std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        if (auto* ptr = get_if< 0>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 1>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 2>(&vars[i])) { sum += read_value(*ptr); continue; }
        //std::unreachable(); // makes the entire benchmark slower
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("get_if", elapsed);

    disable_optimization(sum);
}

template<class Vars>
void benchmark_get_if_16(Table::EntryList& entries, std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 0; i < N; ++i) {
        if (auto* ptr = get_if< 0>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 1>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 2>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 3>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 4>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 5>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 6>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 7>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 8>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if< 9>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if<10>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if<11>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if<12>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if<13>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if<14>(&vars[i])) { sum += read_value(*ptr); continue; }
        if (auto* ptr = get_if<15>(&vars[i])) { sum += read_value(*ptr); continue; }
        //std::unreachable(); // makes the entire benchmark slower
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back("get_if", elapsed);

    disable_optimization(sum);
}

template<class Comp>
consteval std::string_view operator_name()
{
         if constexpr (std::is_same_v<Comp, std::equal_to<>>)        return "operator==";
    else if constexpr (std::is_same_v<Comp, std::not_equal_to<>>)    return "operator!=";
    else if constexpr (std::is_same_v<Comp, std::less<>>)            return "operator<";
    else if constexpr (std::is_same_v<Comp, std::greater<>>)         return "operator>";
    else if constexpr (std::is_same_v<Comp, std::less_equal<>>)      return "operator<=";
    else if constexpr (std::is_same_v<Comp, std::greater_equal<>>)   return "operator>=";
    else if constexpr (std::is_same_v<Comp, std::compare_three_way>) return "operator<=>";
    else static_assert(false);
    std::unreachable();
}

template<class Comp, class Vars>
void benchmark_operator(Table::EntryList& entries, std::size_t const N, Vars const& vars)
{
    unsigned long long sum = 0;

    auto const start_time = Clock::now();
    for (std::size_t i = 1; i < N; ++i) {
        sum += Comp{}(vars[i], vars[i - 1]) == 0;
    }
    auto const end_time = Clock::now();
    auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
    entries.emplace_back(std::string{operator_name<Comp>()}, elapsed);

    disable_optimization(sum);
}

template<class T>
void do_bench(Table& table_3, Table& table_16, std::size_t const N)
{
    table_3.N = N;
    table_16.N = N;

    {
        constexpr std::size_t AltN = 3;
        table_3.AltN = AltN;

        {
            using V = many_V_t<std::variant, AltN, T>;
            auto& entries = table_3.std_datas;

            std::vector<V, yk::default_init_allocator<V>> vars;
            benchmark_construct_3<T>(entries, N, vars);
            benchmark_copy_assign(entries, N, vars);

            benchmark_get_3(entries, N, vars);
            benchmark_get_if_3(entries, N, vars);
            benchmark_visit(entries, N, vars);
            benchmark_multi_visit(entries, N, vars);

            benchmark_operator<std::equal_to<>>(entries, N, vars);
            benchmark_operator<std::not_equal_to<>>(entries, N, vars);
            benchmark_operator<std::less<>>(entries, N, vars);
            benchmark_operator<std::greater<>>(entries, N, vars);
            benchmark_operator<std::less_equal<>>(entries, N, vars);
            benchmark_operator<std::greater_equal<>>(entries, N, vars);
            benchmark_operator<std::compare_three_way>(entries, N, vars);
        }
        {
            using V = many_V_t<yk::rvariant, AltN, T>;
            auto& entries = table_3.rva_datas;

            std::vector<V, yk::default_init_allocator<V>> vars;
            benchmark_construct_3<T>(entries, N, vars);
            benchmark_copy_assign(entries, N, vars);

            benchmark_get_3(entries, N, vars);
            benchmark_get_if_3(entries, N, vars);
            benchmark_visit(entries, N, vars);
            benchmark_multi_visit(entries, N, vars);

            benchmark_operator<std::equal_to<>>(entries, N, vars);
            benchmark_operator<std::not_equal_to<>>(entries, N, vars);
            benchmark_operator<std::less<>>(entries, N, vars);
            benchmark_operator<std::greater<>>(entries, N, vars);
            benchmark_operator<std::less_equal<>>(entries, N, vars);
            benchmark_operator<std::greater_equal<>>(entries, N, vars);
            benchmark_operator<std::compare_three_way>(entries, N, vars);
        }
    }
    {
        constexpr std::size_t AltN = 16;
        table_16.AltN = 16;

        {
            using V = many_V_t<std::variant, AltN, T>;
            auto& entries = table_16.std_datas;

            std::vector<V, yk::default_init_allocator<V>> vars;
            benchmark_construct_16<T>(entries, N, vars);
            benchmark_copy_assign(entries, N, vars);

            benchmark_get_16(entries, N, vars);
            benchmark_get_if_16(entries, N, vars);
            benchmark_visit(entries, N, vars);
            benchmark_multi_visit(entries, N, vars);

            benchmark_operator<std::equal_to<>>(entries, N, vars);
            benchmark_operator<std::not_equal_to<>>(entries, N, vars);
            benchmark_operator<std::less<>>(entries, N, vars);
            benchmark_operator<std::greater<>>(entries, N, vars);
            benchmark_operator<std::less_equal<>>(entries, N, vars);
            benchmark_operator<std::greater_equal<>>(entries, N, vars);
            benchmark_operator<std::compare_three_way>(entries, N, vars);
        }
        {
            using V = many_V_t<yk::rvariant, AltN, T>;
            auto& entries = table_16.rva_datas;

            std::vector<V, yk::default_init_allocator<V>> vars;
            benchmark_construct_16<T>(entries, N, vars);
            benchmark_copy_assign(entries, N, vars);

            benchmark_get_16(entries, N, vars);
            benchmark_get_if_16(entries, N, vars);
            benchmark_visit(entries, N, vars);
            benchmark_multi_visit(entries, N, vars);

            benchmark_operator<std::equal_to<>>(entries, N, vars);
            benchmark_operator<std::not_equal_to<>>(entries, N, vars);
            benchmark_operator<std::less<>>(entries, N, vars);
            benchmark_operator<std::greater<>>(entries, N, vars);
            benchmark_operator<std::less_equal<>>(entries, N, vars);
            benchmark_operator<std::greater_equal<>>(entries, N, vars);
            benchmark_operator<std::compare_three_way>(entries, N, vars);
        }
    }
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

    {
        unsigned long long sum = 0;

        auto const start_time = Clock::now();
        for (std::size_t i = 1; i < N; ++i) {
            sum += N % i;
        }
        auto const end_time = Clock::now();
        auto const elapsed = std::chrono::duration_cast<duration_type>(end_time - start_time);
        std::println("Adding loop-based int {} times: {:.3f} ms", N, elapsed.count());

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
        std::println("Adding random int {} times: {:.3f} ms", N, elapsed.count());

        disable_optimization(sum);
    }
    std::println("");

    // ----------------------------------------------------------

    Table
        int_table_3{"int"}, int_table_16{"int"},
        str_table_3{"std::string"}, str_table_16{"std::string"};

    do_bench<int>(int_table_3, int_table_16, N);
    do_bench<std::string>(str_table_3, str_table_16, std::max(N / 5, 100uz));

    auto const save_csv = [](char const* name, std::string const& csv) {
        std::println("{}", csv);
        std::ofstream ofs(name);
        ofs << csv;
    };
    save_csv("00_int3.csv", int_table_3.make_csv());
    save_csv("01_int16.csv", int_table_16.make_csv());
    save_csv("02_str3.csv", str_table_3.make_csv());
    save_csv("03_str16.csv", str_table_16.make_csv());

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
