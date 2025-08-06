#ifndef YK_RVARIANT_TEST_HPP
#define YK_RVARIANT_TEST_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include "yk/rvariant/rvariant.hpp"
#include "yk/format_traits.hpp"

#include <catch2/catch_test_macros.hpp>

#include <iosfwd>
#include <string>
#include <compare>
#include <functional>
#include <exception>
#include <utility>
#include <format>

#define YK_REQUIRE_STATIC_NOTHROW(...) \
    STATIC_REQUIRE(noexcept(__VA_ARGS__)); \
    REQUIRE_NOTHROW(__VA_ARGS__)

#define YK_CHECK_STATIC_NOTHROW(...) \
    STATIC_CHECK(noexcept(__VA_ARGS__)); \
    CHECK_NOTHROW(__VA_ARGS__)

namespace unit_test {

struct SMF_Logger
{
    std::string log;
    SMF_Logger() { log += "DC "; }
    SMF_Logger(SMF_Logger const&) { log += "CC "; }
    SMF_Logger& operator=(SMF_Logger const&) { log += "CA "; return *this; }
    SMF_Logger(SMF_Logger&&) noexcept(false) { log += "MC "; }
    SMF_Logger& operator=(SMF_Logger&&) noexcept(false) { log += "MA "; return *this; }
};

namespace Thrower_ADL_guard {

namespace detail {

struct Thrower_base
{
private:
    struct non_throwing_t {};
    struct throwing_t {};
    struct potentially_throwing_t {};

public:
    struct exception {}; // NOT derived from std::exception

    static constexpr non_throwing_t non_throwing{};
    static constexpr throwing_t throwing{};
    static constexpr potentially_throwing_t potentially_throwing{};

    Thrower_base() = default;
    Thrower_base(non_throwing_t) noexcept {}
    Thrower_base(throwing_t) noexcept(false) { throw exception{}; }  // NOLINT(hicpp-exception-baseclass)
    Thrower_base(potentially_throwing_t) noexcept(false) {}

    Thrower_base& operator=(non_throwing_t) noexcept { return *this; }
    Thrower_base& operator=(throwing_t) noexcept(false) { throw exception{}; }  // NOLINT(hicpp-exception-baseclass)
    Thrower_base& operator=(potentially_throwing_t) noexcept(false) { return *this; }
};

} // detail

struct Non_Thrower : detail::Thrower_base
{};

struct MC_Thrower : detail::Thrower_base
{
public:
    using MC_Thrower::Thrower_base::Thrower_base;
    using MC_Thrower::Thrower_base::operator=;

    MC_Thrower() noexcept : Thrower_base() {}
    MC_Thrower(MC_Thrower const&) noexcept : Thrower_base() {}
    MC_Thrower(MC_Thrower&&) noexcept(false) : Thrower_base() { throw exception{}; }  // NOLINT(hicpp-exception-baseclass)
    MC_Thrower& operator=(MC_Thrower const&) noexcept { return *this; }
    MC_Thrower& operator=(MC_Thrower&&) noexcept { return *this; }

    friend bool operator==(MC_Thrower const&, MC_Thrower const&) noexcept { return true; }
    friend auto operator<=>(MC_Thrower const&, MC_Thrower const&) noexcept { return std::strong_ordering::equal; }

    //std::string_view dummy{"foobar"};
};

std::ostream& operator<<(std::ostream&, MC_Thrower const&);

} // Thrower_ADL_guard

using Thrower_ADL_guard::detail::Thrower_base;
using Thrower_ADL_guard::Non_Thrower;
using Thrower_ADL_guard::MC_Thrower;


template<class... Ts, class... Args>
[[nodiscard]] constexpr yk::rvariant<Ts..., MC_Thrower> make_valueless(Args&&... args)
{
    static_assert(sizeof...(Ts) > 0);

    using V = yk::rvariant<Ts..., MC_Thrower>;
    static_assert(std::is_nothrow_constructible_v<V, Args...>);
    V a(std::forward<Args>(args)...);

    try {
        V b(std::in_place_type<MC_Thrower>);
        a = std::move(b);
    } catch(...) {  // NOLINT(bugprone-empty-catch)
        if (!a.valueless_by_exception()) {
            throw std::logic_error{"[BUG] exception was thrown, but a is not valueless"};
        }
        return a;
    }

    throw std::logic_error{"[BUG] failed to create valueless instance; exception was never thrown"};
}

template<class T>
struct HashForwarded
{
    T value;
};

}  // unit_test


namespace std {

template<class T>
struct hash<::unit_test::HashForwarded<T>>
{
    size_t operator()(::unit_test::HashForwarded<T> const& v) const
    {
        return std::hash<T>{}(v.value);
    }
};

template<class charT>
struct formatter<::unit_test::MC_Thrower, charT>  // NOLINT(cert-dcl58-cpp)
{
    static constexpr typename std::basic_format_parse_context<charT>::const_iterator
    parse(std::basic_format_parse_context<charT>& ctx)
    {
        if (ctx.begin() == ctx.end()) return ctx.begin();
        if (*ctx.begin() == ::yk::format_traits<charT>::brace_close) return ctx.begin();
        throw std::format_error("MC_Thrower does not accept non-empty format specification");
    }

    template<class OutIt>
    static OutIt format(::unit_test::MC_Thrower const&, std::basic_format_context<OutIt, charT>& ctx)
    {
        return ctx.out();
    }
};

} // std

#endif
