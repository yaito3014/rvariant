#ifndef YK_CORE_CONFIG_HPP
#define YK_CORE_CONFIG_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <version>

#if _MSC_VER
# include <CodeAnalysis/CppCoreCheck/Warnings.h>
# pragma warning(default: CPPCORECHECK_LIFETIME_WARNINGS)
#endif

// <https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#c++20-[[no_unique_address]]>

#if _MSC_VER && _MSC_VER < 1929 // VS 2019 v16.9 or before
# error "Too old MSVC version; we don't support this because it leads to ODR violation regarding the existence of [[(msvc::)no_unique_address]]"
#endif

#if _MSC_VER && __INTELLISENSE__ // Memory Layout view shows wrong layout without this workaround
# define YK_NO_UNIQUE_ADDRESS [[msvc::no_unique_address, no_unique_address]]

#elif _MSC_VER // normal MSVC
# define YK_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]

#else // other compilers
# define YK_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif


#ifndef YK_LIFETIMEBOUND
# if defined(__clang__)
#  define YK_LIFETIMEBOUND [[clang::lifetimebound]]
# elif defined(_MSC_VER)
#  define YK_LIFETIMEBOUND [[msvc::lifetimebound]]
# else
#  define YK_LIFETIMEBOUND
# endif
#endif


#if __cpp_consteval >= 202211L
# define YK_CONSTEXPR_UP constexpr
#else
# define YK_CONSTEXPR_UP consteval
#endif


// TODO:
// [[msvc::forceinline]] https://developercommunity.visualstudio.com/t/support-forceinline-on-c-lambda-expressions/351580#T-N1092115
// [[clang::always_inline]] https://clang.llvm.org/docs/AttributeReference.html#always-inline-force-inline
// [[gnu::always_inline]] https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-always_005finline-function-attribute

#ifndef YK_FORCEINLINE
# ifdef _MSC_VER
#  define YK_FORCEINLINE __forceinline
# elifdef __GNUC__
#  define YK_FORCEINLINE __attribute__((always_inline)) inline
# endif
#endif

#endif
