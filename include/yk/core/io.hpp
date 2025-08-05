#ifndef YK_CORE_IO_HPP
#define YK_CORE_IO_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <iosfwd>
#include <type_traits>
#include <utility>

namespace yk::core {

namespace detail {

namespace ADL_ostreamable_poison_pill {

template<class T>
void operator<<(std::ostream& os, T const&) = delete;

template<class T>
struct ADL_ostreamable_impl : std::false_type {};

template<class T>
    requires requires(std::ostream& os, T const& val) {
        { os << val } -> std::same_as<std::ostream&>;
    }
struct ADL_ostreamable_impl<T> : std::true_type {};

} // ADL_ostreamable_poison_pill

} // detail

template<class T>
struct ADL_ostreamable : detail::ADL_ostreamable_poison_pill::ADL_ostreamable_impl<T> {};

template<class T>
constexpr bool ADL_ostreamable_v = ADL_ostreamable<T>::value;

} // yk::core

#endif
