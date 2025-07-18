#ifndef YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP
#define YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/variant_helper.hpp>
#include <yk/detail/lang_core.hpp>

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace yk::detail {

template<std::size_t I, class T>
struct alternative;

template<bool TriviallyDestructible, class... Ts>
union variadic_union;

template<bool TriviallyDestructible>
union variadic_union<TriviallyDestructible>
{
    constexpr explicit variadic_union(valueless_t) noexcept {}
};

template<bool TriviallyDestructible, class T, class... Ts>
union variadic_union<TriviallyDestructible, T, Ts...>
{
    constexpr explicit variadic_union() noexcept(std::is_nothrow_default_constructible_v<T>)
        : first{} // value-initialize; https://eel.is/c++draft/variant#ctor-3
    {}

    variadic_union(variadic_union const&) = default;
    variadic_union(variadic_union&&) = default;

    constexpr explicit variadic_union(valueless_t) noexcept
        : rest(valueless) // TODO: is this needed?
    {}

    template<class... Args>
    constexpr explicit variadic_union(std::in_place_index_t<0>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : first(std::forward<Args>(args)...)
    {}

    template<std::size_t I, class... Args>
    constexpr explicit variadic_union(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<
            variadic_union<TriviallyDestructible, Ts...>,
            std::in_place_index_t<I - 1>,
            Args...
        >)
        : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...)
    {}

    constexpr ~variadic_union() = default;
    constexpr ~variadic_union() requires (!TriviallyDestructible) {}

    variadic_union& operator=(variadic_union const&) = default;
    variadic_union& operator=(variadic_union&&) = default;

    T first;
    variadic_union<TriviallyDestructible, Ts...> rest;
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
template<> struct alternative<variant_npos, raw_visit_unit_type>
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

template<std::size_t I, class Variant>
[[nodiscard]] constexpr auto&& raw_get(Variant&& var YK_LIFETIMEBOUND) noexcept
{
    return get_alternative<I>{}(std::forward<Variant>(var).storage_);
}

template<std::size_t I, class Variant>
using alternative_t = decltype(raw_get<I>(std::declval<Variant>()));

template<class Visitor, class Variant>
using raw_visit_return_type = std::invoke_result_t<Visitor, alternative_t<0, Variant>>;


template<class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant>
raw_visit_valueless(Visitor&& vis, Variant&&)
    noexcept(std::is_nothrow_invocable_v<Visitor, alternative<variant_npos, raw_visit_unit_type>>)
{
    alternative<variant_npos, raw_visit_unit_type> fake_alternative;
    return std::invoke(std::forward<Visitor>(vis), std::forward_like<Variant>(fake_alternative));
}

template<std::size_t I, class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant>
raw_visit_dispatch(Visitor&& vis, Variant&& var)
    noexcept(std::is_nothrow_invocable_v<Visitor, alternative_t<I, Variant>>)
{
    return std::invoke(std::forward<Visitor>(vis), raw_get<I>(std::forward<Variant>(var)));
}

template<class Visitor, class Variant, class Is = std::make_index_sequence<variant_size_v<std::remove_reference_t<Variant>>>>
constexpr bool raw_visit_noexcept = false;

template<class Visitor, class Variant, std::size_t... Is>
constexpr bool raw_visit_noexcept<Visitor, Variant, std::index_sequence<Is...>> = std::conjunction_v<
    std::is_nothrow_invocable<Visitor, alternative<variant_npos, raw_visit_unit_type>>,
    std::is_nothrow_invocable<Visitor, alternative_t<Is, Variant>>...
>;


template<class Visitor, class Variant, class Seq = std::make_index_sequence<variant_size_v<std::remove_reference_t<Variant>>>>
struct raw_visit_dispatch_table;

template<class Visitor, class Variant, std::size_t... Is>
struct raw_visit_dispatch_table<Visitor, Variant, std::index_sequence<Is...>>
{
    using function_type = raw_visit_return_type<Visitor, Variant> (*)(Visitor&&, Variant&&)
        noexcept(raw_visit_noexcept<Visitor, Variant>);

    static constexpr function_type value[] = {
        &raw_visit_valueless<Visitor, Variant>,
        &raw_visit_dispatch<Is, Visitor, Variant>...
    };
};

template<class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant>
raw_visit(std::size_t index, Visitor&& vis, Variant&& var) noexcept(raw_visit_noexcept<Visitor, Variant>)
{
    constexpr auto const& table = raw_visit_dispatch_table<Visitor, Variant>::value;
    return std::invoke(table[index + 1], std::forward<Visitor>(vis), std::forward<Variant>(var));
}

template<class Seq, class... Ts>
struct variant_storage;

template<std::size_t... Is, class... Ts>
struct variant_storage<std::index_sequence<Is...>, Ts...>
{
    using type = variadic_union<(std::is_trivially_destructible_v<Ts> && ...), alternative<Is, Ts>...>;
};

template<class Seq, class... Ts>
using variant_storage_t = typename variant_storage<Seq, Ts...>::type;

} // yk::detail

#endif
