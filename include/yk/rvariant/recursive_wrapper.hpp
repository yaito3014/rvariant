#ifndef YK_RVARIANT_RECURSIVE_WRAPPER_HPP
#define YK_RVARIANT_RECURSIVE_WRAPPER_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/indirect.hpp>
#include <yk/core/type_traits.hpp>
#include <yk/core/hash.hpp>

#include <compare>
#include <initializer_list>
#include <memory>
#include <utility>

namespace yk {

template<class T, class Allocator = std::allocator<T>>
class recursive_wrapper
    : private yk::indirect<T, Allocator>
{
    static_assert(std::is_object_v<T>);
    static_assert(!std::is_array_v<T>);
    static_assert(!std::is_same_v<T, std::in_place_t>);
    static_assert(!core::is_ttp_specialization_of_v<T, std::in_place_type_t>);
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(std::is_same_v<T, typename std::allocator_traits<Allocator>::value_type>);

    using base_type = yk::indirect<T, Allocator>;

public:
    using typename base_type::allocator_type;
    using typename base_type::const_pointer;
    using typename base_type::pointer;
    using typename base_type::value_type;

    // Don't do this; it breaks third-party analyzer like ReSharper on MSVC
    //using base_type::base_type;

    constexpr /* not explicit */ recursive_wrapper()
        requires std::is_default_constructible_v<Allocator>
    = default;

    // Required for combination with defaulted assignment operators
    constexpr recursive_wrapper(recursive_wrapper const&) = default;
    constexpr recursive_wrapper(recursive_wrapper&&) noexcept = default;

    constexpr explicit recursive_wrapper(std::allocator_arg_t, Allocator const& a)
        noexcept(noexcept(base_type(std::allocator_arg, a)))
        : base_type(std::allocator_arg, a)
    {}

    constexpr recursive_wrapper(std::allocator_arg_t, Allocator const& a, recursive_wrapper const& other)
        noexcept(noexcept(base_type(std::allocator_arg, a, other)))
        : base_type(std::allocator_arg, a, other)
    {}

    constexpr recursive_wrapper(std::allocator_arg_t, Allocator const& a, recursive_wrapper&& other)
        noexcept(noexcept(base_type(std::allocator_arg, a, std::move(other))))
        : base_type(std::allocator_arg, a, std::move(other))
    {}

    // Converting constructor
    template<class U = T>
        requires
            (!std::is_same_v<std::remove_cvref_t<U>, recursive_wrapper>) &&
            (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>) &&
            std::is_default_constructible_v<Allocator> &&
            //std::is_constructible_v<T, U> // UNIMPLEMENTABLE for recursive types; instantiates infinitely
            std::is_convertible_v<U, T>
    constexpr /* not explicit */ recursive_wrapper(U&& u)
        // This overload is never `noexcept` as it always allocates.
        // Note that conditionally enabling `noexcept` will lead to
        // recursive instantiation in some situations.
        : base_type(std::forward<U>(u))
    {}

    template<class U = T>
        requires
            (!std::is_same_v<std::remove_cvref_t<U>, recursive_wrapper>) &&
            (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>) &&
            std::is_constructible_v<T, U>
    constexpr explicit recursive_wrapper(std::allocator_arg_t, Allocator const& a, U&& u)
        noexcept(noexcept(base_type(std::allocator_arg, a, std::forward<U>(u))))
        : base_type(std::allocator_arg, a, std::forward<U>(u))
    {}

    template<class... Us>
        requires
            std::is_constructible_v<T, Us...> &&
            std::is_default_constructible_v<Allocator>
    constexpr explicit recursive_wrapper(std::in_place_t, Us&&... us)
        noexcept(noexcept(base_type(std::in_place, std::forward<Us>(us)...)))
        : base_type(std::in_place, std::forward<Us>(us)...)
    {}

    template<class... Us>
        requires std::is_constructible_v<T, Us...>
    constexpr explicit recursive_wrapper(std::allocator_arg_t, Allocator const& a, std::in_place_t, Us&&... us)
        noexcept(noexcept(base_type(std::allocator_arg, a, std::in_place, std::forward<Us>(us)...)))
        : base_type(std::allocator_arg, a, std::in_place, std::forward<Us>(us)...)
    {}

    template<class I, class... Us>
        requires
            std::is_constructible_v<T, std::initializer_list<I>&, Us...> &&
            std::is_default_constructible_v<Allocator>
    constexpr explicit recursive_wrapper(std::in_place_t, std::initializer_list<I> il, Us&&... us)
        noexcept(noexcept(base_type(std::in_place, il, std::forward<Us>(us)...)))
        : base_type(std::in_place, il, std::forward<Us>(us)...)
    {}

    template<class I, class... Us>
        requires std::is_constructible_v<T, std::initializer_list<I>&, Us...>
    constexpr explicit recursive_wrapper(std::allocator_arg_t, Allocator const& a, std::in_place_t, std::initializer_list<I> il, Us&&... us)
        noexcept(noexcept(base_type(std::allocator_arg, a, std::in_place, il, std::forward<Us>(us)...)))
        : base_type(std::allocator_arg, a, std::in_place, il, std::forward<Us>(us)...)
    {}

    constexpr ~recursive_wrapper() = default;

    // Don't do this; it will lead to surprising result that
    // MSVC attempts to instantiate move assignment operator of *rvariant*
    // when a user just *defines* a struct that contains a rvariant.
    // I don't know why it happens, but MSVC is certainly doing something weird
    // so that it eagerly instantiates unrelated member functions.
    //using base_type::operator=;

    constexpr recursive_wrapper& operator=(recursive_wrapper const&) = default;

    constexpr recursive_wrapper& operator=(recursive_wrapper&&) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
        std::allocator_traits<Allocator>::is_always_equal::value
    ) = default;

    // This is required for proper delegation; otherwise constructor will be called
    template<class U = T>
        requires
            (!std::is_same_v<std::remove_cvref_t<U>, recursive_wrapper>) &&
            std::is_constructible_v<T, U> &&
            std::is_assignable_v<T&, U>
    constexpr recursive_wrapper& operator=(U&& u)
    {
        base_type::operator=(std::forward<U>(u));
        return *this;
    }

    using base_type::operator*;
    using base_type::operator->;
    using base_type::valueless_after_move;
    using base_type::get_allocator;

    using base_type::swap;

    friend constexpr void swap(recursive_wrapper& lhs, recursive_wrapper& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }
};

template<class Value>
recursive_wrapper(Value)
    -> recursive_wrapper<Value>;

template<class Allocator, class Value>
recursive_wrapper(std::allocator_arg_t, Allocator, Value)
    -> recursive_wrapper<Value, typename std::allocator_traits<Allocator>::template rebind_alloc<Value>>;

template<class T, class TA, class U, class UA>
constexpr bool operator==(recursive_wrapper<T, TA> const& lhs, recursive_wrapper<U, UA> const& rhs)
    noexcept(noexcept(*lhs == *rhs))
{
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) [[unlikely]] {
        return lhs.valueless_after_move() == rhs.valueless_after_move();
    } else [[likely]] {
        return *lhs == *rhs;
    }
}

template<class T, class TA, class U, class UA>
constexpr auto operator<=>(recursive_wrapper<T, TA> const& lhs, recursive_wrapper<U, UA> const& rhs) noexcept(core::synth_three_way_noexcept<T, U>)
    -> core::synth_three_way_result_t<T, U>
{
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) [[unlikely]] {
        return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
    } else [[likely]] {
        return core::synth_three_way(*lhs, *rhs);
    }
}

template<class T, class A, class U>
constexpr bool operator==(recursive_wrapper<T, A> const& lhs, U const& rhs)
    noexcept(noexcept(*lhs == rhs))
{
    if (lhs.valueless_after_move()) [[unlikely]] {
        return false;
    } else [[likely]] {
        return *lhs == rhs;
    }
}

template<class T, class A, class U>
constexpr auto operator<=>(recursive_wrapper<T, A> const& lhs, U const& rhs) noexcept(core::synth_three_way_noexcept<T, U>) -> core::synth_three_way_result_t<T, U>
{
    if (lhs.valueless_after_move()) [[unlikely]] {
        return std::strong_ordering::less;
    } else [[likely]] {
        return core::synth_three_way(*lhs, rhs);
    }
}

}  // yk

namespace std {

template<class T, class Allocator>
    requires ::yk::core::is_hash_enabled_v<T>
struct hash<::yk::recursive_wrapper<T, Allocator>>  // NOLINT(cert-dcl58-cpp)
{
    [[nodiscard]] static size_t operator()(::yk::recursive_wrapper<T, Allocator> const& obj)
        noexcept(::yk::core::is_nothrow_hashable_v<T>)
    {
        if (obj.valueless_after_move()) [[unlikely]] {
            return 0xbaddeadbeefuz;
        } else [[likely]] {
            return std::hash<T>{}(*obj);
        }
    }
};

} // std


namespace yk {

template<class T, class Allocator>
    requires core::is_hash_enabled_v<T>
[[nodiscard]] std::size_t hash_value(recursive_wrapper<T, Allocator> const& obj)
    noexcept(core::is_nothrow_hashable_v<T>)
{
    return std::hash<recursive_wrapper<T, Allocator>>{}(obj);
}

} // yk

#endif
