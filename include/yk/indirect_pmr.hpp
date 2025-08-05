#ifndef YK_INDIRECT_PMR_HPP
#define YK_INDIRECT_PMR_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/indirect.hpp>

#include <memory_resource>


namespace yk::pmr {

template<class T>
using indirect = yk::indirect<T, std::pmr::polymorphic_allocator<T>>;

} // yk::pmr

#endif
