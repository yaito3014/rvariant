#ifndef YK_RVARIANT_SUBSET_HPP
#define YK_RVARIANT_SUBSET_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/variant_helper.hpp>
#include <yk/detail/lang_core.hpp>

#include <type_traits>
#include <utility>
#include <variant>

namespace yk {

namespace detail {

template<class From, class To>
struct subset_reindex_impl;

template<
    class... Ts, class... Us
>
struct subset_reindex_impl<type_list<Ts...>, type_list<Us...>>
{
    static constexpr std::size_t table[]{
        find_index_v<Ts, type_list<Us...>>...
    };
};

template<class From, class To>
[[nodiscard]] constexpr std::size_t subset_reindex(std::size_t index) noexcept
{
    return subset_reindex_impl<typename From::unwrapped_types, typename To::unwrapped_types>::table[index];
}

template<class Self, class... Us>
struct subset_visitor
{
    template<class Alt>
    static constexpr rvariant<Us...> operator()([[maybe_unused]] Alt&& alt)
    {
        constexpr std::size_t I = std::remove_cvref_t<Alt>::index;

        if constexpr (I == variant_npos) {
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

namespace rvariant_set {

template<class W, class V>
struct is_subset_of : std::false_type
{
    static_assert(detail::is_ttp_specialization_of_v<W, rvariant>);
    static_assert(detail::is_ttp_specialization_of_v<V, rvariant>);
};

template<class... Us, class... Ts>
struct is_subset_of<rvariant<Us...>, rvariant<Ts...>>
    : std::conjunction<detail::is_in<unwrap_recursive_t<Us>, unwrap_recursive_t<Ts>...>...>
{};

template<class W, class V>
inline constexpr bool is_subset_of_v = is_subset_of<W, V>::value;

template<class W, class V>
concept subset_of = is_subset_of_v<W, V>;

template<class W, class V>
concept equivalent_to = subset_of<W, V> && subset_of<V, W>;

} // rvariant_set

} // yk

#endif
