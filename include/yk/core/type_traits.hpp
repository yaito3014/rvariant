#ifndef YK_CORE_TYPE_TRAITS_HPP
#define YK_CORE_TYPE_TRAITS_HPP

#include <type_traits>
#include <utility>


namespace yk::core {

namespace detail {

// Imaginary function FUN from https://eel.is/c++draft/variant#ctor-14
//   Ti x[] = {std::forward<T>(t)};
// https://eel.is/c++draft/dcl.init.general#14
// https://eel.is/c++draft/dcl.init.list#3.4
// https://eel.is/c++draft/dcl.init.aggr#3

#define YK_RVARIANT_AGGREGATE_INITIALIZE_METHOD_MSVC 0
#define YK_RVARIANT_AGGREGATE_INITIALIZE_METHOD_YK 1
#define YK_RVARIANT_AGGREGATE_INITIALIZE_METHOD YK_RVARIANT_AGGREGATE_INITIALIZE_METHOD_YK

template<std::size_t I, class Ti>
struct aggregate_initialize_tag
{
    static constexpr std::size_t index = I;
    using type = Ti;
};

#if YK_RVARIANT_AGGREGATE_INITIALIZE_METHOD == YK_RVARIANT_AGGREGATE_INITIALIZE_METHOD_MSVC
template<std::size_t I, class Ti>
auto aggregate_initialize_body(Ti(&&)[1]) -> aggregate_initialize_tag<I, Ti>;

template<std::size_t I, class Ti, class T>
using aggregate_initialize_body_t = decltype(
    aggregate_initialize_body<I, Ti>({std::declval<T>()})
);

template<std::size_t I, class Ti>
struct aggregate_initialize_overload
{
    template<class T>
    auto operator()(Ti, T&&) -> aggregate_initialize_body_t<I, Ti, T>;
};

#elif YK_RVARIANT_AGGREGATE_INITIALIZE_METHOD == YK_RVARIANT_AGGREGATE_INITIALIZE_METHOD_YK

// This version works better, does not break IntelliSense or ReSharper
template<std::size_t I, class Ti>
struct aggregate_initialize_overload
{
    using TiA = Ti[];

    template<class T>
    auto operator()(Ti, T&&) -> aggregate_initialize_tag<I, Ti>
        requires requires(T&& t) { { TiA{std::forward<T>(t)} }; }
    {
        return {}; // silence MSVC warning
    }
};
#endif


template<class Is, class... Ts>
struct aggregate_initialize_func_impl;

template<std::size_t... Is, class... Ts>
struct aggregate_initialize_func_impl<std::index_sequence<Is...>, Ts...>
    : aggregate_initialize_overload<Is, Ts>...
{
    using aggregate_initialize_overload<Is, Ts>::operator()...;
};

template<class... Ts>
using aggregate_initialize_func = aggregate_initialize_func_impl<std::index_sequence_for<Ts...>, Ts...>;

template<class Enabled, class T, class... Ts>
struct aggregate_initialize_resolution {};

template<class T, class... Ts>
struct aggregate_initialize_resolution<
    std::void_t<decltype(aggregate_initialize_func<Ts...>{}(std::declval<T>(), std::declval<T>()))>, T, Ts...
> {
    using tag = decltype(aggregate_initialize_func<Ts...>{}(std::declval<T>(), std::declval<T>()));
    using type = typename tag::type;
    static constexpr std::size_t index = tag::index;
};

} // detail

template<class T, class... Ts>
using aggregate_initialize_resolution_t = typename detail::aggregate_initialize_resolution<void, T, Ts...>::type;

template<class T, class... Ts>
constexpr std::size_t aggregate_initialize_resolution_index = detail::aggregate_initialize_resolution<void, T, Ts...>::index;


template<class T, class U, class Enabled = void>
struct is_aggregate_initializable : std::false_type {};

template<class T, class U>
struct is_aggregate_initializable<T, U, std::void_t<decltype(
    std::declval<detail::aggregate_initialize_overload<0, T>>()(std::declval<U>(), std::declval<U>())
)>> : std::true_type
{};

template<class T, class U>
constexpr bool is_aggregate_initializable_v = is_aggregate_initializable<T, U>::value;

} // yk::core

#endif
