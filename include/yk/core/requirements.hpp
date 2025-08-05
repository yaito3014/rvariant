#ifndef YK_CORE_REQUIREMENTS_HPP
#define YK_CORE_REQUIREMENTS_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <concepts>
#include <type_traits>
#include <utility>

namespace yk::core {

namespace detail {

template<class T>
concept boolean_testable_impl = std::convertible_to<T, bool>;

}  // detail

// https://eel.is/c++draft/concept.booleantestable#concept:boolean-testable
template<class T>
concept boolean_testable = detail::boolean_testable_impl<T> && requires(T&& t) {
    { !std::forward<T>(t) } -> detail::boolean_testable_impl;
};

// https://eel.is/c++draft/utility.arg.requirements#tab:cpp17.equalitycomparable
template<class T>
concept Cpp17EqualityComparable =
    requires(T a, T b)             { requires boolean_testable<decltype(a == b)>; } &&
    requires(T const a, T b)       { requires boolean_testable<decltype(a == b)>; } &&
    requires(T a, T const b)       { requires boolean_testable<decltype(a == b)>; } &&
    requires(T const a, T const b) { requires boolean_testable<decltype(a == b)>; };

// https://eel.is/c++draft/utility.requirements#tab:cpp17.lessthancomparable
template<class T>
concept Cpp17LessThanComparable =
    requires(T a, T b)             { requires boolean_testable<decltype(a < b)>; } &&
    requires(T const a, T b)       { requires boolean_testable<decltype(a < b)>; } &&
    requires(T a, T const b)       { requires boolean_testable<decltype(a < b)>; } &&
    requires(T const a, T const b) { requires boolean_testable<decltype(a < b)>; };


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// SEE ALSO: https://cplusplus.github.io/LWG/issue2146
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// https://eel.is/c++draft/utility.requirements#tab:cpp17.defaultconstructible
template<class T>
concept Cpp17DefaultConstructible = requires {
    ::new(static_cast<void*>(nullptr)) T; // T t; is valid
    ::new(static_cast<void*>(nullptr)) T{}; // T u{}; is valid
    T();
    T{};
};

// https://eel.is/c++draft/utility.requirements#tab:cpp17.moveconstructible
template<class T>
concept Cpp17MoveConstructible = requires {
    requires std::is_convertible_v<T, T> && std::is_constructible_v<T, T>;
};

// https://eel.is/c++draft/utility.requirements#tab:cpp17.copyconstructible
template<class T>
concept Cpp17CopyConstructible = Cpp17MoveConstructible<T> && requires {
    requires std::is_convertible_v<T&, T>       && std::is_constructible_v<T, T&>;
    requires std::is_convertible_v<T const&, T> && std::is_constructible_v<T, T const&>;
    requires std::is_convertible_v<T const, T>  && std::is_constructible_v<T, T const>;
};

// https://eel.is/c++draft/utility.requirements#tab:cpp17.moveassignable
template<class T>
concept Cpp17MoveAssignable = requires(T t, T&& rv) {
    { t = static_cast<T&&>(rv) } -> std::same_as<T&>;
};

// https://eel.is/c++draft/utility.requirements#tab:cpp17.copyassignable
template<class T>
concept Cpp17CopyAssignable = Cpp17MoveAssignable<T> && requires {
    requires requires(T t, T& v)        { { t = v } -> std::same_as<T&>; };
    requires requires(T t, T const& v)  { { t = v } -> std::same_as<T&>; };
    requires requires(T t, T const&& v) { { t = static_cast<T const&&>(v) } -> std::same_as<T&>; };
};

// https://eel.is/c++draft/utility.requirements#tab:cpp17.destructible
template<class T>
concept Cpp17Destructible = (!std::is_array_v<T>) && std::is_object_v<T> && requires(T u) {
    { u.~T() };
};

// https://eel.is/c++draft/swappable.requirements#5
template<class X>
concept Cpp17Swappable = std::is_swappable_v<X&>;

} // yk::core

#endif
