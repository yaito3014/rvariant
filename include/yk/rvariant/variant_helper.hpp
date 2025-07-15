#ifndef YK_RVARIANT_VARIANT_HELPER_HPP
#define YK_RVARIANT_VARIANT_HELPER_HPP

// Utilities related to [rvariant.helper], [rvariant.recursive.helper]

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/detail/lang_core.hpp>

#include <utility>
#include <type_traits>
#include <cstddef>

namespace yk {

namespace detail {

using variant_index_t = std::size_t; // TODO: make this select the cheap type

// Intentionally defined in the `detail` to avoid confusion with `std::variant_npos`.
// Equals to std::variant_npos by definition
inline constexpr variant_index_t variant_npos = static_cast<variant_index_t>(-1);

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
[[nodiscard]] constexpr decltype(auto)
unwrap_recursive(T&& o YK_LIFETIMEBOUND) noexcept
{
    if constexpr (yk::detail::is_ttp_specialization_of_v<std::remove_cvref_t<T>, recursive_wrapper>) {
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
struct variant_alternative<I, rvariant<Ts...>> : yk::detail::pack_indexing<I, unwrap_recursive_t<Ts>...>
{
    static_assert(I < sizeof...(Ts));
};

} // yk

#endif
