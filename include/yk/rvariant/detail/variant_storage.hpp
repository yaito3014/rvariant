#ifndef YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP
#define YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/variant_helper.hpp>

#include <functional>
#include <utility>

namespace yk::detail {

template<bool TriviallyDestructible, class... Ts>
struct variadic_union {};

template<class... Ts>
using make_variadic_union_t = variadic_union<std::conjunction_v<std::is_trivially_destructible<Ts>...>, Ts...>;


template<class T, class... Ts>
struct variadic_union<true, T, Ts...>
{
    static_assert(true == std::conjunction_v<std::is_trivially_destructible<T>, std::is_trivially_destructible<Ts>...>);

    static constexpr std::size_t size = sizeof...(Ts) + 1;

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

    template<class... Args>
        requires std::is_constructible_v<T, Args...> // required for not confusing some compilers
    constexpr explicit variadic_union(std::in_place_index_t<0>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : first(std::forward<Args>(args)...) // value-initialize; https://eel.is/c++draft/variant#ctor-3
    {}

    template<std::size_t I, class... Args>
        // required for not confusing some compilers
        requires
            (I != 0) &&
            std::is_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>
    constexpr explicit variadic_union(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<
            make_variadic_union_t<Ts...>,
            std::in_place_index_t<I - 1>,
            Args...
        >)
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

    template<class... Args>
        requires std::is_constructible_v<T, Args...> // required for not confusing some compilers
    constexpr explicit variadic_union(std::in_place_index_t<0>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : first(std::forward<Args>(args)...) // value-initialize; https://eel.is/c++draft/variant#ctor-3
    {}

    template<std::size_t I, class... Args>
        // required for not confusing some compilers
        requires
            (I != 0) &&
            std::is_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>
    constexpr explicit variadic_union(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<
            make_variadic_union_t<Ts...>,
            std::in_place_index_t<I - 1>,
            Args...
        >)
        : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...)
    {}

    union {
        T first;
        make_variadic_union_t<Ts...> rest;
    };
};

// TODO: apply same logic to other visits with >= O(n^2) branch
inline constexpr std::size_t visit_instantiation_limit = 1024;

template<std::size_t I>
struct get_alternative
{
    static_assert(I != variant_npos);

    template<class Union>
    static constexpr auto&& apply(Union&& u YK_LIFETIMEBOUND) noexcept
    {
        return get_alternative<I - 1>::apply(std::forward<Union>(u).rest);
    }
};

template<>
struct get_alternative<0>
{
    template<class Union>
    static constexpr auto&& apply(Union&& u YK_LIFETIMEBOUND) noexcept
    {
        return std::forward<Union>(u).first;
    }
};

template<std::size_t I, class Storage>
[[nodiscard]] constexpr decltype(auto) raw_get(Storage&& storage YK_LIFETIMEBOUND) noexcept
{
    return get_alternative<I>::apply(std::forward<Storage>(storage));
}

template<std::size_t I, class Storage>
using raw_get_t = decltype(raw_get<I>(std::declval<Storage>()));


template<class Visitor, class Storage, class Is = std::make_index_sequence<std::remove_cvref_t<Storage>::size>>
struct raw_visit_return_type_impl;

template<class Visitor, class Storage, std::size_t I>
struct raw_visit_return_type_impl<Visitor, Storage, std::index_sequence<I>>
{
    static_assert(
        std::is_invocable_v<Visitor, std::in_place_index_t<I>, raw_get_t<I, Storage>>,
        "The spec requires Visitor to accept all alternative types (with the value category being identical)." // https://eel.is/c++draft/variant#visit-5
    );

    using type = std::invoke_result_t<Visitor, std::in_place_index_t<I>, raw_get_t<I, Storage>>;
};

template<class Visitor, class Storage, std::size_t I, std::size_t... Is> requires (sizeof...(Is) > 0)
struct raw_visit_return_type_impl<Visitor, Storage, std::index_sequence<I, Is...>>
    : raw_visit_return_type_impl<Visitor, Storage, std::index_sequence<Is...>>
{
    static_assert(
        std::conjunction_v<std::is_same<
            std::invoke_result_t<Visitor, std::in_place_index_t<I>, raw_get_t<I, Storage>>,
            std::invoke_result_t<Visitor, std::in_place_index_t<Is>, raw_get_t<Is, Storage>>
        >...>,
        "The spec requires the Visitor to return the same type and value category for all alternatives." // https://eel.is/c++draft/variant#visit-5
    );
};

template<class Visitor, class Storage>
using raw_visit_return_type = typename raw_visit_return_type_impl<Visitor, Storage>::type;


template<class Visitor, class Storage>
constexpr raw_visit_return_type<Visitor, Storage>
raw_visit_valueless(Visitor&& vis, Storage&&)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    noexcept(std::is_nothrow_invocable_v<Visitor, std::in_place_index_t<variant_npos>, decltype(std::forward_like<Storage>(std::declval<valueless_t>()))>)
{
    valueless_t valueless_;
    return std::invoke(std::forward<Visitor>(vis), std::in_place_index<variant_npos>, std::forward_like<Storage>(valueless_));
}

template<std::size_t I, class Visitor, class Storage>
constexpr raw_visit_return_type<Visitor, Storage>
raw_visit_dispatch(Visitor&& vis, Storage&& storage)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    noexcept(std::is_nothrow_invocable_v<Visitor, std::in_place_index_t<I>, raw_get_t<I, Storage>>)
{
    return std::invoke(std::forward<Visitor>(vis), std::in_place_index<I>, raw_get<I>(std::forward<Storage>(storage)));
}

template<class Visitor, class Storage, class Is = std::make_index_sequence<std::remove_cvref_t<Storage>::size>>
constexpr bool raw_visit_noexcept = false;

template<class Visitor, class Storage, std::size_t... Is>
constexpr bool raw_visit_noexcept<Visitor, Storage, std::index_sequence<Is...>> = std::conjunction_v<
    std::is_nothrow_invocable<Visitor, std::in_place_index_t<variant_npos>, decltype(std::forward_like<Storage>(std::declval<valueless_t>()))>,
    std::is_nothrow_invocable<Visitor, std::in_place_index_t<Is>, raw_get_t<Is, Storage>>...
>;


template<class Visitor, class Storage, class Seq = std::make_index_sequence<std::remove_cvref_t<Storage>::size>>
struct raw_visit_dispatch_table;

template<class Visitor, class Storage>
using raw_visit_function_ptr = raw_visit_return_type<Visitor, Storage>(*) (Visitor&&, Storage&&)
    noexcept(raw_visit_noexcept<Visitor, Storage>);

template<class Visitor, class Storage, std::size_t... Is>
struct raw_visit_dispatch_table<Visitor, Storage, std::index_sequence<Is...>>
{
    static_assert(std::is_reference_v<Visitor>, "Visitor must be one of: &, const&, &&, const&&");
    static_assert(std::is_reference_v<Storage>, "Storage must be one of: &, const&, &&, const&&");

    static constexpr raw_visit_function_ptr<Visitor, Storage> value[] = {
        &raw_visit_valueless<Visitor, Storage>,
        &raw_visit_dispatch<Is, Visitor, Storage>...
    };
};

} // yk::detail

#endif
