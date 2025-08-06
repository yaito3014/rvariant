#ifndef YK_RVARIANT_DETAIL_RVARIANT_FWD_HPP
#define YK_RVARIANT_DETAIL_RVARIANT_FWD_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <cstddef>

#if !defined(YK_RVARIANT_VISIT_STRENGTHEN)
# define YK_RVARIANT_VISIT_STRENGTHEN 1
#endif

#if YK_RVARIANT_VISIT_STRENGTHEN
# define YK_RVARIANT_VISIT_NOEXCEPT(...) noexcept(__VA_ARGS__)
#else
# define YK_RVARIANT_VISIT_NOEXCEPT(...)
#endif

namespace yk {

template<class... Ts>
class rvariant;

template<class T, class Allocator>
class recursive_wrapper;


namespace detail {

struct valueless_t
{
    constexpr explicit valueless_t() = default;
};

inline constexpr valueless_t valueless{};

} // detail

} // yk

#endif
