#ifndef YK_CORE_HASH_FWD_HPP
#define YK_CORE_HASH_FWD_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <version>

#if __cplusplus <= 202302L || \
    (defined(_MSVC_STL_UPDATE) && _MSVC_STL_UPDATE < 202504L) || \
    (defined(__GLIBCXX__) && __GLIBCXX__ < 20241201) || \
    (defined(_LIBCPP_STD_VER) && _LIBCPP_STD_VER < 26)

# include <typeindex> // pre-C++26
#else
# include <utility> // C++26 and later
#endif

#endif
