#ifndef YK_INDIRECT_PMR_HPP
#define YK_INDIRECT_PMR_HPP

#include <yk/indirect.hpp>

#include <memory_resource>


namespace yk::pmr {

template<class T>
using indirect = yk::indirect<T, std::pmr::polymorphic_allocator<T>>;

} // yk::pmr

#endif
