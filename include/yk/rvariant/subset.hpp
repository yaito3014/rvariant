#ifndef YK_RVARIANT_SUBSET_HPP
#define YK_RVARIANT_SUBSET_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/variant_helper.hpp>
#include <yk/core/type_traits.hpp>

#include <type_traits>

namespace yk {

namespace detail {

template<class From, class To>
struct subset_reindex_impl;

template<class... Ts, class... Us>
struct subset_reindex_impl<core::type_list<Ts...>, core::type_list<Us...>>
{
    static constexpr std::size_t table[]{
        core::find_index_v<Ts, core::type_list<Us...>>...
    };
};

template<class From, class To>
[[nodiscard]] consteval std::size_t subset_reindex(std::size_t index) noexcept
{
    return subset_reindex_impl<typename From::unwrapped_types, typename To::unwrapped_types>::table[index];
}

} // detail


namespace rvariant_set {

template<class W, class V>
struct is_subset_of : std::false_type
{
    static_assert(core::is_ttp_specialization_of_v<W, rvariant>);
    static_assert(core::is_ttp_specialization_of_v<V, rvariant>);
};

template<class... Us, class... Ts>
struct is_subset_of<rvariant<Us...>, rvariant<Ts...>>
    : std::conjunction<
        std::disjunction<
            core::is_in<Us, Ts...>,
            core::is_in<Us, unwrap_recursive_t<Ts>...>
        >...
    >
{};

// subset_of<int, int> => true
// subset_of<int, recursive_wrapper<int>> => true
// subset_of<recursive_wrapper<int>, recursive_wrapper<int>> => true
// subset_of<recursive_wrapper<int>, int> => false

template<class W, class V>
inline constexpr bool is_subset_of_v = is_subset_of<W, V>::value;

template<class W, class V>
concept subset_of = is_subset_of_v<W, V>;

template<class W, class V>
concept equivalent_to = subset_of<W, V> && subset_of<V, W>;

} // rvariant_set

} // yk

#endif
