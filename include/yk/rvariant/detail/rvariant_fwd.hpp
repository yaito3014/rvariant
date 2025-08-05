#ifndef YK_RVARIANT_DETAIL_RVARIANT_FWD_HPP
#define YK_RVARIANT_DETAIL_RVARIANT_FWD_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <cstddef>


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
