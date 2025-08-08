#ifndef YK_RVARIANT_RVARIANT_IO_HPP
#define YK_RVARIANT_RVARIANT_IO_HPP

// Copyright 2025 Nana Sakisaka
// Copyright 2025 Yaito Kakeyama
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/rvariant.hpp>
#include <yk/core/io.hpp>
#include <yk/format_traits.hpp>

#include <format>
// ReSharper disable once CppUnusedIncludeDirective
#include <ostream>

namespace yk {

namespace detail {

[[nodiscard]] inline bool set_iostate_check_rethrow(std::ostream& os, std::ios_base::iostate const state) noexcept
{
    // https://eel.is/c++draft/ostream.formatted.reqmts#footnote-285
    if ((os.exceptions() & state) != 0) {
        // hard mode -- the standard provides no method to set the state
        // _without_ throwing `std::ios_base::failure`, so we must
        // hide it.
        try {
            os.setstate(state);
        } catch (std::ios_base::failure const&) {  // NOLINT(bugprone-empty-catch)
        }
        return true;

    } else {
        os.setstate(state);
        return false;
    }
}

} // detail


// Behaves mostly like *formatted output function* (https://eel.is/c++draft/ostream.formatted.reqmts),
// except that `std::bad_variant_access` will always be propagated.
template<class... Ts>
    requires std::conjunction_v<core::ADL_ostreamable<unwrap_recursive_t<Ts>>...>
std::ostream& operator<<(std::ostream& os, rvariant<Ts...> const& v)
{
    std::ostream::sentry sentry(os);
    if (!sentry) {
        os.setstate(std::ios_base::badbit);
        return os;
    }

    if (v.valueless_by_exception()) [[unlikely]] {
        // does not set badbit, as per the spec
        detail::throw_bad_variant_access(); // always throw, regardless of `os.exceptions()`
    }

    try {
        detail::raw_visit(v, [&os]<std::size_t i>(std::in_place_index_t<i>, [[maybe_unused]] auto const& o) {
            if constexpr (i == std::variant_npos) {
                std::unreachable();
            } else {
                os << detail::unwrap_recursive(o); // NOTE: this may also throw `std::bad_variant_access`
            }
        });

    } catch (std::bad_variant_access const&) {
        // does not set badbit, as per the spec
        throw; // always throw, regardless of `os.exceptions()`

    } catch (...) {
        bool const need_rethrow = detail::set_iostate_check_rethrow(os, std::ios_base::badbit);
        if (need_rethrow) throw;
    }
    return os;
}

// ----------------------------------------------------
// std::formatter support

namespace detail {

template<class charT, class T>
struct variant_format_string_overload
{
    using fmt_type = std::basic_format_string<charT, T const&>;
    fmt_type fmt;

    [[nodiscard]] constexpr fmt_type const& operator()(std::in_place_type_t<T>) const noexcept
    {
        return fmt;
    }
};

template<class... Fs>
struct variant_format_string : Fs...
{
    static_assert(sizeof...(Fs) > 0);
    using Fs::operator()...;
};

template<class charT, class... Ts>
using variant_format_string_t = variant_format_string<variant_format_string_overload<charT, Ts>...>;

template<class VFormat, class Variant>
struct variant_format_proxy
{
    static_assert(!std::is_rvalue_reference_v<VFormat>);
    static_assert(!std::is_rvalue_reference_v<Variant>);
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    VFormat v_fmt;
    Variant v;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

template<class charT, class Variant>
struct variant_format_for_impl;

template<class charT, class... Ts>
struct variant_format_for_impl<charT, rvariant<Ts...>>
{
    using spec_type = variant_format_string_t<charT, Ts...>;

    template<class... Fmts>
    [[nodiscard]] static YK_CONSTEXPR_UP spec_type
    apply(Fmts&&... fmts) noexcept
    {
        return variant_format_string{variant_format_string_overload<charT, Ts>{
            std::forward<Fmts>(fmts)
        }...};
    }
};

}  // detail


template<class... Ts, class... Fmts, class charT = select_char_t<Fmts...>>
[[nodiscard]] YK_CONSTEXPR_UP detail::variant_format_string_t<charT, Ts...>
variant_format(Fmts&&... fmts) noexcept
{
    static_assert(sizeof...(Ts) > 0);
    return detail::variant_format_string{detail::variant_format_string_overload<charT, Ts>{
        std::forward<Fmts>(fmts)
    }...};
}

template<class Variant, class... Fmts, class charT = select_char_t<Fmts...>>
[[nodiscard]] YK_CONSTEXPR_UP typename detail::variant_format_for_impl<charT, std::remove_cvref_t<Variant>>::spec_type
variant_format_for(Fmts&&... fmts) noexcept
{
    static_assert(core::is_ttp_specialization_of_v<std::remove_cvref_t<Variant>, rvariant>);
    return detail::variant_format_for_impl<charT, std::remove_cvref_t<Variant>>::apply(
        std::forward<Fmts>(fmts)...
    );
}

template<class VFormat, class Variant>
    requires
        core::is_ttp_specialization_of_v<std::remove_cvref_t<VFormat>, detail::variant_format_string> &&
        core::is_ttp_specialization_of_v<std::remove_cvref_t<Variant>, rvariant>
[[nodiscard]] constexpr detail::variant_format_proxy<VFormat, Variant>
format_by(VFormat&& v_fmt YK_LIFETIMEBOUND, Variant&& v YK_LIFETIMEBOUND) noexcept
{
    return detail::variant_format_proxy<VFormat, Variant>{
        std::forward<VFormat>(v_fmt), std::forward<Variant>(v)
    };
}

namespace detail {

template<class charT, class Variant>
struct variant_alts_formattable : std::false_type
{
    static_assert(core::is_ttp_specialization_of_v<Variant, rvariant>);
};

template<class charT, class... Ts>
struct variant_alts_formattable<charT, rvariant<Ts...>>
    : std::bool_constant<(std::formattable<unwrap_recursive_t<Ts>, charT> && ...)>
{};

} // detail

}  // yk


namespace std {

template<class... Ts, class charT>
    requires (std::formattable<::yk::unwrap_recursive_t<Ts>, charT> && ...)
struct formatter<::yk::rvariant<Ts...>, charT>  // NOLINT(cert-dcl58-cpp)
{
    static constexpr typename std::basic_format_parse_context<charT>::const_iterator
    parse(std::basic_format_parse_context<charT>& ctx)
    {
        if (ctx.begin() == ctx.end()) return ctx.begin();
        if (*ctx.begin() == ::yk::format_traits<charT>::brace_close) return ctx.begin();
        throw std::format_error(
            "rvariant itself only accepts empty format spec `{}`; use "
            "`format_by` or manually dispatch alternatives in `visit` "
            "if you need more controls."
        );
    }

    template<class OutIt>
    static OutIt format(::yk::rvariant<Ts...> const& v, std::basic_format_context<OutIt, charT>& ctx)
    {
        return ::yk::detail::raw_visit(
            v,
            [&]<std::size_t i, class VT>(std::in_place_index_t<i>, VT const& alt) -> OutIt {
                if constexpr (i == std::variant_npos) {
                    (void)alt;
                    ::yk::detail::throw_bad_variant_access();
                } else {
                    return std::format_to(
                        ctx.out(),
                        ::yk::format_traits<charT>::template brace_full<::yk::unwrap_recursive_t<VT> const&>,
                        ::yk::detail::unwrap_recursive(alt)
                    );
                }
            }
        );
    }
};

template<class VFormat, class Variant, class charT>
    requires
        ::yk::core::is_ttp_specialization_of_v<std::remove_cvref_t<VFormat>, ::yk::detail::variant_format_string> &&
        ::yk::core::is_ttp_specialization_of_v<std::remove_cvref_t<Variant>, ::yk::rvariant> &&
        ::yk::detail::variant_alts_formattable<charT, std::remove_cvref_t<Variant>>::value
struct formatter<::yk::detail::variant_format_proxy<VFormat, Variant>, charT>  // NOLINT(cert-dcl58-cpp)
{
    static constexpr typename std::basic_format_parse_context<charT>::const_iterator
    parse(std::basic_format_parse_context<charT>& ctx)
    {
        if (ctx.begin() == ctx.end()) return ctx.begin();
        if (*ctx.begin() == ::yk::format_traits<charT>::brace_close) return ctx.begin();
        throw std::format_error(
            "format_by only accepts empty format spec; use "
            "`variant_format` for full controls."
        );
    }

    template<class OutIt>
    static OutIt format(::yk::detail::variant_format_proxy<VFormat, Variant> const& proxy, std::basic_format_context<OutIt, charT>& ctx)
    {
        return ::yk::detail::raw_visit(
            proxy.v,
            [&]<std::size_t i, class VT>(std::in_place_index_t<i>, VT const& alt) -> OutIt {
                if constexpr (i == std::variant_npos) {
                    (void)alt;
                    ::yk::detail::throw_bad_variant_access();
                } else {
                    static_assert(
                        std::is_invocable_v<VFormat, std::in_place_type_t<::yk::unwrap_recursive_t<VT>>>,
                        "`VFormat` must provide format string for all alternative types."
                    );
                    return std::format_to(
                        ctx.out(),
                        std::invoke(proxy.v_fmt, std::in_place_type<::yk::unwrap_recursive_t<VT>>),
                        ::yk::detail::unwrap_recursive(alt)
                    );
                }
            }
        );
    }
};

}  // namespace std

#endif
