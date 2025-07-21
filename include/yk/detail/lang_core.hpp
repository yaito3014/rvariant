#ifndef YK_DETAIL_LANG_CORE_HPP
#define YK_DETAIL_LANG_CORE_HPP

#include <type_traits>
#include <concepts>
#include <cstddef>

#if _MSC_VER
# include <CppCoreCheck/Warnings.h>
# pragma warning(default: CPPCORECHECK_LIFETIME_WARNINGS)
#endif

// <https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#c++20-[[no_unique_address]]>

#if _MSC_VER && _MSC_VER < 1929 // VS 2019 v16.9 or before
# error "Too old MSVC version; we don't support this because it leads to ODR violation regarding the existence of [[(msvc::)no_unique_address]]"
#endif

#if _MSC_VER && __INTELLISENSE__ // Memory Layout view shows wrong layout without this workaround
# define YK_NO_UNIQUE_ADDRESS [[msvc::no_unique_address, no_unique_address]]

#elif _MSC_VER // normal MSVC
# define YK_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]

#else // other compilers
# define YK_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif


#ifndef YK_LIFETIMEBOUND
# if defined(__clang__)
#  define YK_LIFETIMEBOUND [[clang::lifetimebound]]
# elif defined(_MSC_VER)
#  define YK_LIFETIMEBOUND [[msvc::lifetimebound]]
# else
#  define YK_LIFETIMEBOUND
# endif
#endif


namespace yk::detail {

// Provides equivalent definition as MSVC's STL for semantic compatibility
namespace has_ADL_swap_detail {

#if defined(__clang__) || defined(__EDG__)
void swap() = delete; // poison pill
#else
void swap();
#endif

template<class, class = void> struct has_ADL_swap : std::false_type {};
template<class T> struct has_ADL_swap<T, std::void_t<decltype(swap(std::declval<T&>(), std::declval<T&>()))>> : std::true_type {};

} // has_ADL_swap_detail

template<class T>
constexpr bool is_trivially_swappable_v = std::conjunction_v<
    std::is_trivially_destructible<T>,
    std::is_trivially_move_constructible<T>,
    std::is_trivially_move_assignable<T>,
    // std::is_swappable cannot be used for this purpose because it assumes `using std::swap`
    std::negation<has_ADL_swap_detail::has_ADL_swap<T>>
>;

template<> inline constexpr bool is_trivially_swappable_v<std::byte> = true;
template<class T> struct is_trivially_swappable : std::bool_constant<is_trivially_swappable_v<T>> {};


template<class T, template<class...> class TT>
struct is_ttp_specialization_of : std::false_type {};

template<template<class...> class TT, class... Ts>
struct is_ttp_specialization_of<TT<Ts...>, TT> : std::true_type {};

template<class T, template<class...> class TT>
inline constexpr bool is_ttp_specialization_of_v = is_ttp_specialization_of<T, TT>::value;

template<class T, template<class...> class TT>
concept ttp_specialization_of = is_ttp_specialization_of_v<T, TT>;


template<class T, template<auto...> class TT>
struct is_nttp_specialization_of : std::false_type {};

template<template<auto...> class TT, auto... Ts>
struct is_nttp_specialization_of<TT<Ts...>, TT> : std::true_type {};

template<class T, template<auto...> class TT>
inline constexpr bool is_nttp_specialization_of_v = is_nttp_specialization_of<T, TT>::value;

template<class T, template<auto...> class TT>
concept nttp_specialization_of = is_nttp_specialization_of_v<T, TT>;


template<class... Ts>
struct type_list {};


template<std::size_t I, class... Ts>
struct pack_indexing;

template<std::size_t I, class... Ts>
using pack_indexing_t = typename pack_indexing<I, Ts...>::type;

template<class T, class... Ts>
struct pack_indexing<0, T, Ts...> { using type = T; };

template<std::size_t I, class T, class... Ts>
struct pack_indexing<I, T, Ts...> : pack_indexing<I - 1, Ts...> {};


inline constexpr std::size_t find_npos = -1;

template<std::size_t I, class T, class... Ts>
struct find_index_impl
    : std::integral_constant<std::size_t, find_npos>
{};

template<std::size_t I, class T, class U, class... Us>
struct find_index_impl<I, T, U, Us...>
    : std::conditional_t<
        std::is_same_v<T, U>,
        std::integral_constant<std::size_t, I>,
        find_index_impl<I + 1, T, Us...>
    >
{};

template<class T, class List>
struct find_index;

template<class T, template<class...> class TT, class... Ts>
struct find_index<T, TT<Ts...>> : find_index_impl<0, T, Ts...> {};

template<class T, class List>
inline constexpr std::size_t find_index_v = find_index<T, List>::value;


template<class T, class... Ts>
struct is_in : std::disjunction<std::is_same<T, Ts>...> {};

template<class T, class... Ts>
inline constexpr bool is_in_v = is_in<T, Ts...>::value;


template<template<class A, class B> class F, class U, class... Ts>
struct disjunction_for : std::disjunction<F<U, Ts>...> {};

template<template<class A, class B> class F, class U, class... Ts>
inline constexpr bool disjunction_for_v = disjunction_for<F, U, Ts...>::value;

template<template<class A, class B> class F, class U, class... Ts>
struct conjunction_for : std::conjunction<F<U, Ts>...> {};

template<template<class A, class B> class F, class U, class... Ts>
inline constexpr bool conjunction_for_v = conjunction_for<F, U, Ts...>::value;


template<bool Found, class T, class... Us>
struct exactly_once_impl : std::bool_constant<Found> {};

template<class T, class U, class... Us>
struct exactly_once_impl<false, T, U, Us...>
    : exactly_once_impl<std::is_same_v<T, U>, T, Us...> {};

template<class T, class U, class... Us>
struct exactly_once_impl<true, T, U, Us...>
    : std::conditional_t<std::is_same_v<T, U>, std::false_type, exactly_once_impl<true, T, Us...>> {};

template<class T, class List>
struct exactly_once;

template<class T, template<class...> class TT, class... Ts>
struct exactly_once<T, TT<Ts...>> : exactly_once_impl<false, T, Ts...>
{
    static_assert(sizeof...(Ts) > 0);
};

template<class T, class List>
inline constexpr bool exactly_once_v = exactly_once<T, List>::value;

} // yk::detail

#endif
