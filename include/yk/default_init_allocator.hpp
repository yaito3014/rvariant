// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef YK_DEFAULT_INIT_ALLOCATOR_HPP
#define YK_DEFAULT_INIT_ALLOCATOR_HPP

#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace yk {

// http://stackoverflow.com/a/21028912/273767

template<class T, class A = std::allocator<T>>
class default_init_allocator : public A
{
    using traits = std::allocator_traits<A>;

public:
    template<class U>
    struct rebind
    {
        using other = default_init_allocator<U, typename traits::template rebind_alloc<U>>;
    };

    using A::A;

    template<class U>
    constexpr void construct(U* ptr) noexcept(std::is_nothrow_default_constructible_v<U>)
    {
        ::new (static_cast<void*>(ptr)) U;
    }

    template<class U, class... Args>
    constexpr void construct(U* ptr, Args&&... args)
        noexcept(noexcept(traits::construct(static_cast<A&>(*this), ptr, std::forward<Args>(args)...)))
    {
        traits::construct(static_cast<A&>(*this), ptr, std::forward<Args>(args)...);
    }
};

} // yk

#endif
