#ifndef YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP
#define YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/variant_helper.hpp>

#include <functional>
#include <utility>
#include <type_traits>

#include <cassert>

#if defined(_MSC_VER)
# define YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN \
    _Pragma("warning(push)") \
    _Pragma("warning(disable: 4702)")
# define YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END \
    _Pragma("warning(pop)")
#else
# define YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
# define YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END
#endif

namespace yk::detail {

template<bool NeverValueless, class T>
[[nodiscard]] YK_FORCEINLINE constexpr std::size_t valueless_bias(T i) noexcept
{
    if constexpr (NeverValueless) {
        return i;
    } else {
        return ++i;
    }
}

template<class Variant, class T>
[[nodiscard]] YK_FORCEINLINE constexpr std::size_t valueless_bias(T i) noexcept
{
    if constexpr (std::remove_cvref_t<Variant>::never_valueless) {
        return i;
    } else {
        return ++i;
    }
}

template<bool NeverValueless, class T>
[[nodiscard]] YK_FORCEINLINE constexpr std::size_t valueless_unbias(T i) noexcept
{
    if constexpr (NeverValueless) {
        return i;
    } else {
        return --i;
    }
}

template<class Variant, class T>
[[nodiscard]] YK_FORCEINLINE constexpr std::size_t valueless_unbias(T i) noexcept
{
    if constexpr (std::remove_cvref_t<Variant>::never_valueless) {
        return i;
    } else {
        return --i;
    }
}

// Any non type-changing operation
//   => never becomes valueless (delegates to underlying type's exception safety)
//
// Construction
//   => no need to consider type traits, because lifetime never starts on exception
//
// Assignment (type-changing & RHS is not valueless)
//   => valueless iff move constructor throws
//
// Emplace (if VT(Args...) is throwing)
//   rvariant tmp(std::in_place_index<I>, std::forward<Args>(args)...);
//   *this = std::move(tmp);
//        ^^^ needs to be NOT observable on user's part, as per "Effects" https://eel.is/c++draft/variant.mod#7
//                        ^^^^^^^^^^^^^^
//                           if type-changing: VT is trivially move constructible
//                       if NOT type-changing: VT is trivially move assignable
//                                    ... and trivially destructible.
// So the final condition is:
//    move constructor is noexcept && (<= discarded; weaker than triviality)
//    trivially move constructible &&
//    trivially move assignable &&
//    trivially destructible.
//
// Note that "move" operation can fall back to "copy" if "move" is
// non-trivial AND "copy" is trivial.
//
// Furthermore, we have modified the spec for `.emplace` so that
// `recursive_wrapper` can be always treated as never_valueless part,
// so we include that optimization for PoC.

// Additional size limit
inline constexpr std::size_t never_valueless_trivial_size_limit = 256;

template<class... Ts>
struct is_never_valueless
    : std::conjunction<
        std::disjunction<
            core::is_ttp_specialization_of<Ts, recursive_wrapper>,
            std::conjunction<
                std::bool_constant<sizeof(Ts) <= never_valueless_trivial_size_limit>,
                std::is_trivially_destructible<Ts>,
                std::disjunction<std::is_trivially_move_constructible<Ts>, std::is_trivially_copy_constructible<Ts>>,
                std::disjunction<std::is_trivially_move_assignable<Ts>, std::is_trivially_copy_assignable<Ts>>
            >
        >...
    >
{
    static_assert(sizeof...(Ts) > 0);
};

template<class... Ts>
constexpr bool is_never_valueless_v = is_never_valueless<Ts...>::value;

// for bypassing access control
template<class Storage>
struct storage_never_valueless : std::bool_constant<std::remove_cvref_t<Storage>::never_valueless> {};

// --------------------------------------------

template<bool TriviallyDestructible, class... Ts>
struct variadic_union {};

template<class... Ts>
using make_variadic_union_t = variadic_union<std::conjunction_v<std::is_trivially_destructible<Ts>...>, Ts...>;


template<class T, class... Ts>
struct variadic_union<true, T, Ts...>
{
    static_assert(true == std::conjunction_v<std::is_trivially_destructible<T>, std::is_trivially_destructible<Ts>...>);

    static constexpr std::size_t size = sizeof...(Ts) + 1;
    static constexpr bool never_valueless = is_never_valueless_v<T, Ts...>;

    // no active member
    // ReSharper disable once CppPossiblyUninitializedMember
    constexpr explicit variadic_union() noexcept {}

#ifdef __RESHARPER__
    // These are required for propagating traits
    variadic_union(variadic_union const&) = default;
    variadic_union(variadic_union&&) = default;
    variadic_union& operator=(variadic_union const&) = default;
    variadic_union& operator=(variadic_union&&) = default;

    // According to the standard, a union with non-trivially-(copy|move)-(constructible|assignable)
    // members has *implicitly*-deleted corresponding special functions.
    // Although it should work only by the "= default" declaration, some compilers
    // (e.g. ReSharper's "Code Inspection") fail to detect such traits,
    // resulting in red squiggles everywhere.
    variadic_union(variadic_union const&)            requires((!std::conjunction_v<std::is_trivially_copy_constructible<T>, std::is_trivially_copy_constructible<Ts>...>)) = delete;
    variadic_union(variadic_union&&)                 requires((!std::conjunction_v<std::is_trivially_move_constructible<T>, std::is_trivially_move_constructible<Ts>...>)) = delete;
    variadic_union& operator=(variadic_union const&) requires((!std::conjunction_v<std::is_trivially_copy_assignable<T>, std::is_trivially_copy_assignable<Ts>...>)) = delete;
    variadic_union& operator=(variadic_union&&)      requires((!std::conjunction_v<std::is_trivially_move_assignable<T>, std::is_trivially_move_assignable<Ts>...>)) = delete;
#endif

YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
    template<class... Args>
        requires std::is_constructible_v<T, Args...> // required for not confusing some compilers
    constexpr explicit variadic_union(std::in_place_index_t<0>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : first(std::forward<Args>(args)...) // value-initialize; https://eel.is/c++draft/variant.ctor#3
    {}
YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END

    template<std::size_t I, class... Args>
        requires (I != 0) && std::is_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>
    constexpr explicit variadic_union(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>)
        : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...)
    {}

    union {
        T first;
        make_variadic_union_t<Ts...> rest;
    };
};

template<class T, class... Ts>
struct variadic_union<false, T, Ts...>
{
    static_assert(false == std::conjunction_v<std::is_trivially_destructible<T>, std::is_trivially_destructible<Ts>...>);

    static constexpr std::size_t size = sizeof...(Ts) + 1;
    static constexpr bool never_valueless = is_never_valueless_v<T, Ts...>;

    // no active member
    // ReSharper disable once CppPossiblyUninitializedMember
    constexpr explicit variadic_union() noexcept {}

    constexpr ~variadic_union() noexcept {}

    variadic_union(variadic_union const&) = default;
    variadic_union(variadic_union&&) = default;
    variadic_union& operator=(variadic_union const&) = default;
    variadic_union& operator=(variadic_union&&) = default;

#ifdef __RESHARPER__
    variadic_union(variadic_union const&)            requires((!std::conjunction_v<std::is_trivially_copy_constructible<T>, std::is_trivially_copy_constructible<Ts>...>)) = delete;
    variadic_union(variadic_union&&)                 requires((!std::conjunction_v<std::is_trivially_move_constructible<T>, std::is_trivially_move_constructible<Ts>...>)) = delete;
    variadic_union& operator=(variadic_union const&) requires((!std::conjunction_v<std::is_trivially_copy_assignable<T>, std::is_trivially_copy_assignable<Ts>...>)) = delete;
    variadic_union& operator=(variadic_union&&)      requires((!std::conjunction_v<std::is_trivially_move_assignable<T>, std::is_trivially_move_assignable<Ts>...>)) = delete;
#endif

YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
    template<class... Args>
        requires std::is_constructible_v<T, Args...> // required for not confusing some compilers
    constexpr explicit variadic_union(std::in_place_index_t<0>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : first(std::forward<Args>(args)...) // value-initialize; https://eel.is/c++draft/variant.ctor#3
    {}
YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END

    template<std::size_t I, class... Args>
        requires (I != 0) && std::is_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>
    constexpr explicit variadic_union(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>)
        : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...)
    {}

    union {
        T first;
        make_variadic_union_t<Ts...> rest;
    };
};

template<class Variant>
struct forward_storage_t_impl
{
    static_assert(
        core::is_ttp_specialization_of_v<std::remove_cvref_t<Variant>, rvariant>,
        "`forward_storage` only accepts types which are exactly `rvariant`. Maybe you forgot `as_rvariant_t`?"
    );
    using type = decltype(std::declval<Variant>().storage());
};

template<class Variant>
using forward_storage_t = typename forward_storage_t_impl<Variant>::type;

template<class Variant>
[[nodiscard]] YK_FORCEINLINE constexpr forward_storage_t<Variant>&&
forward_storage(std::remove_reference_t<Variant>& v YK_LIFETIMEBOUND) noexcept
{
    return std::forward<Variant>(v).storage();
}

template<class Variant>
[[nodiscard]] YK_FORCEINLINE constexpr forward_storage_t<Variant>&&
forward_storage(std::remove_reference_t<Variant>&& v YK_LIFETIMEBOUND) noexcept  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    return std::forward<Variant>(v).storage();
}


template<std::size_t I, class Storage>
[[nodiscard]] YK_FORCEINLINE constexpr auto&& raw_get(Storage&& storage YK_LIFETIMEBOUND) noexcept
{
         if constexpr (I ==  0) return std::forward<Storage>(storage).first;
    else if constexpr (I ==  1) return std::forward<Storage>(storage).rest.first;
    else if constexpr (I ==  2) return std::forward<Storage>(storage).rest.rest.first;
    else if constexpr (I ==  3) return std::forward<Storage>(storage).rest.rest.rest.first;
    else if constexpr (I ==  4) return std::forward<Storage>(storage).rest.rest.rest.rest.first;
    else if constexpr (I ==  5) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.first;
    else if constexpr (I ==  6) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I ==  7) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I ==  8) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I ==  9) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 10) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 11) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 12) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 13) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 14) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 15) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 16) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 17) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 18) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 19) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 20) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 21) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 22) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 23) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 24) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 25) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 26) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 27) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 28) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 29) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 30) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I == 31) return std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.first;
    else if constexpr (I < 64)  return raw_get<I - 32>(
                                       std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest);
    else                        return raw_get<I - 64>(
                                       std::forward<Storage>(storage).rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest
                                                                     .rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest.rest);
}

template<std::size_t I, class Storage>
using raw_get_t = decltype(raw_get<I>(std::declval<Storage>()));

// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

// https://eel.is/c++draft/variant.visit

namespace as_variant_impl {

template<class... Ts>
constexpr auto&& as_variant(rvariant<Ts...>& var) { return var; }

template<class... Ts>
constexpr auto&& as_variant(rvariant<Ts...> const& var) { return var; }

template<class... Ts>
constexpr auto&& as_variant(rvariant<Ts...>&& var) { return std::move(var); }

template<class... Ts>
constexpr auto&& as_variant(rvariant<Ts...> const&& var) { return std::move(var); }

} // as_variant_impl

template<class T>
using as_variant_t = decltype(as_variant_impl::as_variant(std::declval<T>()));

} // yk::detail

#endif
