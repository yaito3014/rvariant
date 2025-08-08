#ifndef YK_RVARIANT_BENCHMARK_SUPPORT_HPP
#define YK_RVARIANT_BENCHMARK_SUPPORT_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <memory>
#include <cstdint>

namespace benchmark {

#ifdef _WIN32
# define YK_BENCHMARK_API __declspec(dllexport)
#else
# define YK_BENCHMARK_API __attribute__((visibility("default")))
#endif

namespace detail {

YK_BENCHMARK_API void disable_optimization_impl(void const*) noexcept;
YK_BENCHMARK_API void asm_trace(std::uint_least32_t line) noexcept;

} // detail

#define YK_ASM_TRACE ::benchmark::detail::asm_trace(__LINE__);

template<class T>
void disable_optimization(T const& v) noexcept
{
    detail::disable_optimization_impl(std::addressof(v));
}

}

#endif
