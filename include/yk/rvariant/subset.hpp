#ifndef YK_RVARIANT_SUBSET_HPP
#define YK_RVARIANT_SUBSET_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>

#include <type_traits>
#include <utility>
#include <variant>

namespace yk {

namespace detail {

template<class From, class To>
struct subset_reindex_impl;

template<
    class... Ts,
    class... Us
>
struct subset_reindex_impl<rvariant<Ts...>, rvariant<Us...>>
{
    static constexpr std::size_t table[]{find_index_v<Ts, type_list<Us...>>...};
    static constexpr std::size_t apply(std::size_t index) noexcept { return table[index]; }
};

template<class From, class To>
[[nodiscard]] constexpr std::size_t subset_reindex(std::size_t index) noexcept
{
    return subset_reindex_impl<From, To>::apply(index);
}

template<class Self, class... Us>
struct subset_visitor
{
    template<class Alt>
    static constexpr rvariant<Us...> operator()(Alt&& alt)
    {
        constexpr std::size_t I = std::remove_cvref_t<Alt>::index;

        if constexpr (I == std::variant_npos) {
            return rvariant<Us...>(valueless);

        } else {
            constexpr std::size_t pos = subset_reindex<Self, rvariant<Us...>>(I);
            if constexpr (pos == find_index_npos) {
                throw std::bad_variant_access{};

            } else {
                return rvariant<Us...>(std::in_place_index<pos>, std::forward<Alt>(alt).value);
            }
        }
    }
};

} // detail


template<class T, class U>
struct is_subset_of : std::false_type {};

template<template<class...> class L1, class... Ts, template<class...> class L2, class... Us>
struct is_subset_of<L1<Ts...>, L2<Us...>> : std::conjunction<detail::is_in<Ts, Us...>...> {};

template<class T, class U>
inline constexpr bool is_subset_of_v = is_subset_of<T, U>::value;

template<class T, class U>
concept subset_of = is_subset_of_v<T, U>;

template<class T, class U>
concept equivalent_to = subset_of<T, U> && subset_of<U, T>;

} // yk

#endif
