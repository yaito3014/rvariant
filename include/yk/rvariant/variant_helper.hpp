#ifndef YK_RVARIANT_VARIANT_HELPER_HPP
#define YK_RVARIANT_VARIANT_HELPER_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Utilities related to [rvariant.helper], [rvariant.recursive.helper]

// This header is intended to be extremely lightweight;
// third-party codes may depend on only this header to
// provide sufficient concepts for their code.

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/core/type_traits.hpp>

#include <utility>
#include <type_traits>
#include <cstddef>

namespace yk {

namespace detail {

template<std::size_t VariantSize>
struct variant_index_selector
{
    using type = int;
};

template<std::size_t VariantSize>
    requires (VariantSize <= static_cast<std::size_t>(static_cast<unsigned char>(-1) / 2))
struct variant_index_selector<VariantSize>
{
    using type = signed char;
};

template <std::size_t VariantSize>
    requires (static_cast<std::size_t>(static_cast<unsigned char>(-1) / 2) < VariantSize && VariantSize <= static_cast<std::size_t>(static_cast<unsigned short>(-1) / 2))
struct variant_index_selector<VariantSize>
{
    using type = short;
};

template<std::size_t VariantSize>
using variant_index_t = typename variant_index_selector<VariantSize>::type;

// Intentionally defined in the `detail` to avoid confusion with `std::variant_npos`.
// Equals to std::variant_npos by definition
template<std::size_t VariantSize>
inline constexpr variant_index_t<VariantSize> variant_npos = static_cast<variant_index_t<VariantSize>>(-1);

template<std::size_t I, class T>
struct alternative;

} // detail


template<class Variant>
struct variant_size;

template<class Variant>
inline constexpr std::size_t variant_size_v = variant_size<Variant>::value;

template<class Variant>
struct variant_size<Variant const> : variant_size<Variant> {};

template<class... Ts>
struct variant_size<rvariant<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};


namespace detail {

template<class T>
[[nodiscard]] YK_FORCEINLINE constexpr auto&&
unwrap_recursive(T&& o YK_LIFETIMEBOUND) noexcept
{
    if constexpr (core::is_ttp_specialization_of_v<std::remove_cvref_t<T>, recursive_wrapper>) {
        return *std::forward<T>(o);
    } else {
        return std::forward<T>(o);
    }
}

}  // detail

template<class T> struct unwrap_recursive { using type = T; };
template<class T, class Allocator> struct unwrap_recursive<recursive_wrapper<T, Allocator>> { using type = T; };
template<class T> using unwrap_recursive_t = typename unwrap_recursive<T>::type;


template<std::size_t I, class Variant>
struct variant_alternative;

template<std::size_t I, class Variant>
using variant_alternative_t = typename variant_alternative<I, Variant>::type;

template<std::size_t I, class Variant>
struct variant_alternative<I, Variant const> : std::add_const<variant_alternative_t<I, Variant>> {};

template<std::size_t I, class... Ts>
struct variant_alternative<I, rvariant<Ts...>> : core::pack_indexing<I, unwrap_recursive_t<Ts>...>
{
    static_assert(I < sizeof...(Ts));
};


template<class... Fs>
struct overloaded : Fs...
{
    using Fs::operator()...;
};


namespace detail {

template<class VT, class RHS>
struct forward_maybe_wrapped_impl; // [rvariant.rvariant.general]: different allocators are not allowed

// recursive_wrapper<int> val = recursive_wrapper<int>{42};
template<class T, class Allocator>
struct forward_maybe_wrapped_impl<recursive_wrapper<T, Allocator>, recursive_wrapper<T, Allocator>>
{
    template<class Wrapped>
    [[nodiscard]] static constexpr auto&& apply(Wrapped&& wrapped YK_LIFETIMEBOUND) noexcept
    {
        static_assert(std::is_same_v<std::remove_cvref_t<Wrapped>, recursive_wrapper<T, Allocator>>);
        return std::forward<Wrapped>(wrapped);
    }
};

// recursive_wrapper<int> val = 42;
template<class T, class Allocator>
struct forward_maybe_wrapped_impl<recursive_wrapper<T, Allocator>, T>
{
    template<class Value>
    [[nodiscard]] static constexpr auto&& apply(Value&& value YK_LIFETIMEBOUND) noexcept
    {
        static_assert(std::is_same_v<std::remove_cvref_t<Value>, T>);
        return std::forward<Value>(value);
    }
};

// int val = 42;
template<class T>
struct forward_maybe_wrapped_impl<T, T>
{
    template<class Value>
    [[nodiscard]] static constexpr auto&& apply(Value&& value YK_LIFETIMEBOUND) noexcept
    {
        static_assert(std::is_same_v<std::remove_cvref_t<Value>, T>);
        return std::forward<Value>(value);
    }
};

template<class VT, class RHS>
[[nodiscard]] constexpr auto&& forward_maybe_wrapped(RHS&& rhs YK_LIFETIMEBOUND) noexcept
{
    static_assert(!std::is_reference_v<VT> && !std::is_const_v<VT>);
    return forward_maybe_wrapped_impl<VT, std::remove_cvref_t<RHS>>::apply(std::forward<RHS>(rhs));
}

} // detail

} // yk

#endif
