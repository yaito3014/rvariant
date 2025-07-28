#ifndef YK_RVARIANT_RECURSIVE_WRAPPER_PMR_HPP
#define YK_RVARIANT_RECURSIVE_WRAPPER_PMR_HPP

#include <yk/rvariant/recursive_wrapper.hpp>

#include <memory_resource>

namespace yk::pmr {

template<class T>
using recursive_wrapper = yk::recursive_wrapper<T, std::pmr::polymorphic_allocator<T>>;

} // yk::pmr

#endif
