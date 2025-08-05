#ifndef YK_RVARIANT_RECURSIVE_WRAPPER_PMR_HPP
#define YK_RVARIANT_RECURSIVE_WRAPPER_PMR_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/recursive_wrapper.hpp>

#include <memory_resource>

namespace yk::pmr {

template<class T>
using recursive_wrapper = yk::recursive_wrapper<T, std::pmr::polymorphic_allocator<T>>;

} // yk::pmr

#endif
