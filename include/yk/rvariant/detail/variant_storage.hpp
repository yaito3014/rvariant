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
        : first()
    {}

    variadic_union(variadic_union const&) = default;
    variadic_union(variadic_union&&) = default;

    constexpr explicit variadic_union(valueless_t) noexcept
        : rest(valueless)
    {}

    template<class... Args>
    constexpr explicit variadic_union(std::in_place_index_t<0>, Args&&... args)
        : first(std::forward<Args>(args)...)
    {}

    template<std::size_t I, class... Args>
    constexpr explicit variadic_union(std::in_place_index_t<I>, Args&&... args)
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

    constexpr explicit alternative() = default;

    template<class... Args>
        requires (sizeof...(Args) > 0) && std::is_constructible_v<T, Args...>
    constexpr explicit alternative(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : value(std::forward<Args>(args)...)
    {}

    T value;
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
        return std::forward<Union>(u); // valueless_t
    }
};

template<std::size_t I, class Variant>
constexpr auto&& raw_get(Variant&& var YK_LIFETIMEBOUND) noexcept
{
    return get_alternative<I>{}(std::forward<Variant>(var).storage_);
}

template<std::size_t I, class Variant>
using alternative_t = decltype(raw_get<I>(std::declval<Variant>()));

template<class Visitor, class Variant>
using raw_visit_return_type = std::invoke_result_t<Visitor, alternative_t<0, Variant>>;

template<std::size_t I, class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant>
raw_visit_dispatch(Visitor&& vis, Variant&& var)
    noexcept(std::is_nothrow_invocable_v<Visitor, alternative_t<0, Variant>>)
{
    return std::invoke(std::forward<Visitor>(vis), raw_get<I>(std::forward<Variant>(var)));
}

struct raw_visit_unit_type {};

template<class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant>
raw_visit_valueless(Visitor&& vis, Variant&&)
    noexcept(std::is_nothrow_invocable_v<Visitor, alternative<variant_npos, raw_visit_unit_type>>)
{
    return std::invoke(std::forward<Visitor>(vis), alternative<variant_npos, raw_visit_unit_type>{});
}

template<class Visitor, class Variant, class Seq = std::make_index_sequence<variant_size_v<std::remove_reference_t<Variant>>>>
struct raw_visit_dispatch_table;

template<class Visitor, class Variant, std::size_t... Is>
struct raw_visit_dispatch_table<Visitor, Variant, std::index_sequence<Is...>>
{
    using function_type = raw_visit_return_type<Visitor, Variant> (*)(Visitor&&, Variant&&);
    static constexpr function_type value[] = {
        &raw_visit_valueless<Visitor, Variant>,
        &raw_visit_dispatch<Is, Visitor, Variant>...
    };
};

template<class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant>
raw_visit(std::size_t index, Visitor&& vis, Variant&& var)
    noexcept(std::is_nothrow_invocable_v<Visitor, alternative_t<0, Variant>>)
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
