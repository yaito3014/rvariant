#ifndef YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP
#define YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/variant_helper.hpp>
#include <yk/core/type_traits.hpp>

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

template<std::size_t I, class T>
struct alternative
{
    static constexpr std::size_t index = I;
    using type = T;

    constexpr explicit alternative() noexcept(std::is_nothrow_default_constructible_v<T>)
        = default;

    template<class... Args>
        requires (sizeof...(Args) > 0) && std::is_constructible_v<T, Args...>
    constexpr explicit alternative(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : value(std::forward<Args>(args)...)
    {}

    T value;
};

struct raw_visit_unit_type {};

template<>
struct alternative<variant_npos, raw_visit_unit_type>
{
    static constexpr std::size_t index = variant_npos;
};


template<std::size_t I>
struct get_alternative
{
    template<class Union>
    constexpr auto&& operator()(Union&& u YK_LIFETIMEBOUND) noexcept
    {
        return get_alternative<I - 1>{}(std::forward<Union>(u).rest);
    }
};

template<>
struct get_alternative<0>
{
    template<class Union>
    constexpr auto&& operator()(Union&& u YK_LIFETIMEBOUND) noexcept
    {
        return std::forward<Union>(u).first;
    }
};

template<>
struct get_alternative<variant_npos>
{
    template<class Union>
    constexpr auto&& operator()(Union&& u YK_LIFETIMEBOUND) noexcept
    {
        return std::forward<Union>(u); // valueless storage itself
    }
};


template<std::size_t I, class Storage>
[[nodiscard]] constexpr auto&& raw_get(Storage&& storage YK_LIFETIMEBOUND) noexcept
{
    return get_alternative<I>{}(std::forward<Storage>(storage));
}

template<std::size_t I, class Storage>
using alternative_t = decltype(raw_get<I>(std::declval<Storage>()));

template<class Visitor, class Storage>
using raw_visit_return_type = std::invoke_result_t<Visitor, alternative_t<0, Storage>>;


template<class Visitor, class Storage>
constexpr raw_visit_return_type<Visitor, Storage>
raw_visit_valueless(Visitor&& vis, Storage&&)
    noexcept(std::is_nothrow_invocable_v<Visitor, alternative<variant_npos, raw_visit_unit_type>>)
{
    alternative<variant_npos, raw_visit_unit_type> fake_alternative;
    return std::invoke(std::forward<Visitor>(vis), std::forward_like<Storage>(fake_alternative));
}

template<std::size_t I, class Visitor, class Storage>
constexpr raw_visit_return_type<Visitor, Storage>
raw_visit_dispatch(Visitor&& vis, Storage&& storage)
    noexcept(std::is_nothrow_invocable_v<Visitor, alternative_t<I, Storage>>)
{
    return std::invoke(std::forward<Visitor>(vis), raw_get<I>(std::forward<Storage>(storage)));
}

template<class Visitor, class Storage, class Is = std::make_index_sequence<std::remove_cvref_t<Storage>::size>>
constexpr bool raw_visit_noexcept = false;

template<class Visitor, class Storage, std::size_t... Is>
constexpr bool raw_visit_noexcept<Visitor, Storage, std::index_sequence<Is...>> = std::conjunction_v<
    std::is_nothrow_invocable<Visitor, alternative<variant_npos, raw_visit_unit_type>>,
    std::is_nothrow_invocable<Visitor, alternative_t<Is, Storage>>...
>;


template<class Visitor, class Storage, class Seq = std::make_index_sequence<std::remove_cvref_t<Storage>::size>>
struct raw_visit_dispatch_table;

template<class Visitor, class Storage>
using raw_visit_function_ptr = raw_visit_return_type<Visitor, Storage>(*) (Visitor&&, Storage&&)
    noexcept(raw_visit_noexcept<Visitor, Storage>);

template<class Visitor, class Storage, std::size_t... Is>
struct raw_visit_dispatch_table<Visitor, Storage, std::index_sequence<Is...>>
{
    static constexpr raw_visit_function_ptr<Visitor, Storage> value[] = {
        &raw_visit_valueless<Visitor, Storage>,
        &raw_visit_dispatch<Is, Visitor, Storage>...
    };
};


template<class Seq, class... Ts>
struct variant_storage_for_impl;

template<std::size_t... Is, class... Ts>
struct variant_storage_for_impl<std::index_sequence<Is...>, Ts...>
{
    // sanity check
    static_assert(
        ((std::is_trivially_destructible_v<alternative<Is, Ts>> == std::is_trivially_destructible_v<Ts>) && ...)
    );

    using type = variadic_union<
        //(std::is_trivially_destructible_v<Ts> && ...),
        (std::is_trivially_destructible_v<alternative<Is, Ts>> && ...),
        alternative<Is, Ts>...
    >;
};

template<class... Ts>
using variant_storage_for = typename variant_storage_for_impl<std::index_sequence_for<Ts...>, Ts...>::type;

} // yk::detail

#endif
