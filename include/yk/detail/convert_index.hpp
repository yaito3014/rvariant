#ifndef YK_DETAIL_CONVERT_INDEX_HPP
#define YK_DETAIL_CONVERT_INDEX_HPP

#include "yk/detail/find_index.hpp"

#include <cstddef>

namespace yk::detail {

template<class From, class To>
struct convert_index_impl;

template<template<class...> class L1, class... Ts, template<class...> class L2, class... Us>
struct convert_index_impl<L1<Ts...>, L2<Us...>> {
    static constexpr std::size_t table[]{find_index_v<Ts, Us...>...};
    static constexpr std::size_t apply(std::size_t index) noexcept { return table[index]; }
};

template<class From, class To>
constexpr std::size_t convert_index(std::size_t index) noexcept {
    return convert_index_impl<From, To>::apply(index);
}

}  // namespace yk::detail

#endif  // YK_DETAIL_CONVERT_INDEX_HPP
