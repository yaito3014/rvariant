#ifndef YK_CORE_CONCEPTS_HPP
#define YK_CORE_CONCEPTS_HPP

#include <concepts>
#include <utility>

namespace yk::core {

namespace detail {

template<class T>
concept boolean_testable_impl = std::convertible_to<T, bool>;

}  // detail

template<class T>
concept boolean_testable = detail::boolean_testable_impl<T> && requires(T&& t) {
    { !std::forward<T>(t) } -> detail::boolean_testable_impl;
};

}  // yk::core

#endif  // YK_CORE_CONCEPTS_HPP
