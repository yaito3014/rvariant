// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "benchmark_support.hpp"

namespace benchmark {

namespace detail {

YK_BENCHMARK_API void disable_optimization_impl(void const*) noexcept
{
    // do nothing
}

YK_BENCHMARK_API void asm_trace(std::uint_least32_t) noexcept
{
    // do nothing
}

} // detail

} // benchmark
