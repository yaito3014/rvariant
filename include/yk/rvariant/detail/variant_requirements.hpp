#ifndef YK_RVARIANT_DETAIL_VARIANT_REQUIREMENTS_HPP
#define YK_RVARIANT_DETAIL_VARIANT_REQUIREMENTS_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/detail/rvariant_fwd.hpp>

#include <yk/core/type_traits.hpp>


namespace yk::detail {

template<class T, class U>
struct check_recursive_wrapper_duplicate_impl : std::true_type {};

template<class T, class Allocator>
struct check_recursive_wrapper_duplicate_impl<recursive_wrapper<T, Allocator>, T>
    : std::false_type
{
    // ReSharper disable once CppStaticAssertFailure
    static_assert(
        false,
        "rvariant cannot contain both `T` and `recursive_wrapper<T, Allocator>` "
        "([rvariant.rvariant.general])."
    );
};

template<class T, class Allocator>
struct check_recursive_wrapper_duplicate_impl<T, recursive_wrapper<T, Allocator>>
    : std::false_type
{
    // ReSharper disable once CppStaticAssertFailure
    static_assert(
        false,
        "rvariant cannot contain both `T` and `recursive_wrapper<T, Allocator>` "
        "([rvariant.rvariant.general])."
    );
};

template<class T, class Allocator, class UAllocator>
    requires (!std::is_same_v<Allocator, UAllocator>)
struct check_recursive_wrapper_duplicate_impl<recursive_wrapper<T, Allocator>, recursive_wrapper<T, UAllocator>>
    : std::false_type
{
    // ReSharper disable once CppStaticAssertFailure
    static_assert(
        false,
        "rvariant cannot contain multiple different allocator specializations of "
        "`recursive_wrapper` for the same `T` ([rvariant.rvariant.general])."
    );
};

template<class T, class... Ts>
struct check_recursive_wrapper_duplicate : std::true_type {};

template<class T, class... Ts> requires (sizeof...(Ts) > 0)
struct check_recursive_wrapper_duplicate<T, T, Ts...>
    : std::conjunction<check_recursive_wrapper_duplicate_impl<T, Ts>...>
{};

template<class T, class List>
struct non_wrapped_exactly_once : core::exactly_once<T, List>
{
    static_assert(
        !core::is_ttp_specialization_of_v<T, recursive_wrapper>,
        "Constructing a `recursive_wrapper` alternative with its full type as the tag is "
        "prohibited to avoid confusion; just specify `T` instead."
    );
};

template<class T, class List>
constexpr bool non_wrapped_exactly_once_v = non_wrapped_exactly_once<T, List>::value;


template<class T, class Variant>
struct exactly_once_index
{
    static_assert(core::exactly_once_v<T, typename Variant::unwrapped_types>, "T or recursive_wrapper<T> must occur exactly once in Ts...");
    static constexpr std::size_t value = core::find_index_v<T, typename Variant::unwrapped_types>;
};

template<class T, class Variant>
inline constexpr std::size_t exactly_once_index_v = exactly_once_index<T, Variant>::value;


template<class T, class U = T const&>
struct variant_copy_assignable : std::conjunction<std::is_constructible<T, U>, std::is_assignable<T&, U>>
{
    static_assert(!std::is_reference_v<T>);
    static_assert(std::is_lvalue_reference_v<U>);
};

template<class T, class U = T const&>
struct variant_nothrow_copy_assignable : std::conjunction<std::is_nothrow_constructible<T, U>, std::is_nothrow_assignable<T&, U>>
{
    static_assert(!std::is_reference_v<T>);
    static_assert(std::is_lvalue_reference_v<U>);
    static_assert(variant_copy_assignable<T, U>::value);
};

template<class T, class U = T&&>
struct variant_move_assignable : std::conjunction<std::is_constructible<T, U>, std::is_assignable<T&, U>>
{
    static_assert(!std::is_reference_v<T>);
    static_assert(std::is_rvalue_reference_v<U>);
};

template<class T, class U = T&&>
struct variant_nothrow_move_assignable : std::conjunction<std::is_nothrow_constructible<T, U>, std::is_nothrow_assignable<T&, U>>
{
    static_assert(!std::is_reference_v<T>);
    static_assert(std::is_rvalue_reference_v<U>);
    static_assert(variant_move_assignable<T, U>::value);
};

template<class T, class U>
struct variant_assignable : std::conjunction<std::is_constructible<T, U>, std::is_assignable<T&, U>>
{
    static_assert(!std::is_reference_v<T>);
};

template<class T, class U>
struct variant_nothrow_assignable : std::conjunction<std::is_nothrow_constructible<T, U>, std::is_nothrow_assignable<T&, U>>
{
    static_assert(!std::is_reference_v<T>);
    static_assert(variant_assignable<T, U>::value);
};

} // yk::detail

#endif
