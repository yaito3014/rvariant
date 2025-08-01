#ifndef YK_RVARIANT_RVARIANT_IO_HPP
#define YK_RVARIANT_RVARIANT_IO_HPP

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
        throw std::bad_variant_access(); // always throw, regardless of `os.exceptions()`
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
struct format_spec_overload
{
    using fmt_type = std::basic_format_string<charT, T const&>;
    fmt_type fmt;

    [[nodiscard]] constexpr fmt_type const& operator()(std::in_place_type_t<T>) const noexcept
    {
        return fmt;
    }
};

template<class... Fs>
struct format_spec : Fs...
{
    static_assert(sizeof...(Fs) > 0);
    using Fs::operator()...;
};

template<class charT, class... Ts>
using format_spec_t = format_spec<format_spec_overload<charT, Ts>...>;

template<class FormatSpec, class... Ts>
struct format_by_proxy
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    FormatSpec fspec;
    rvariant<Ts...> const& v;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
};

template<class charT, class Variant>
struct make_format_spec_for_impl;

template<class charT, class... Ts>
struct make_format_spec_for_impl<charT, rvariant<Ts...>>
{
    using spec_type = format_spec_t<charT, Ts...>;

    [[nodiscard]] static consteval spec_type
    apply(std::basic_format_string<charT, Ts const&> const&... fmts) noexcept
    {
        return format_spec{format_spec_overload<charT, Ts>{fmts}...};
    }
};

}  // detail


template<class charT, class... Ts>
[[nodiscard]] consteval detail::format_spec_t<charT, Ts...>
make_format_spec(std::basic_format_string<charT, Ts const&> const&... fmts) noexcept
{
    return detail::format_spec{detail::format_spec_overload<charT, Ts>{fmts}...};
}

template<class Variant, class... Fmts>
[[nodiscard]] consteval typename detail::make_format_spec_for_impl<select_char_t<Fmts...>, Variant>::spec_type
make_format_spec_for(Fmts const&... fmts) noexcept
{
    return detail::make_format_spec_for_impl<select_char_t<Fmts...>, Variant>::apply(fmts...);
}

template<class FormatSpec, class... Ts>
[[nodiscard]] constexpr detail::format_by_proxy<FormatSpec, Ts...>
format_by(FormatSpec&& fspec YK_LIFETIMEBOUND, rvariant<Ts...> const& var YK_LIFETIMEBOUND)
{
    return detail::format_by_proxy<FormatSpec, Ts...>{
        std::forward<FormatSpec>(fspec), var
    };
}

template<class FormatSpec, class... Ts>
void format_by(FormatSpec&&, rvariant<Ts...>&&) = delete; // dangling

template<class FormatSpec, class... Ts>
void format_by(FormatSpec&&, rvariant<Ts...> const&&) = delete; // dangling

}  // yk


namespace std {

template<class FormatSpec, class... Ts, class charT>
struct formatter<yk::detail::format_by_proxy<FormatSpec, Ts...>, charT>  // NOLINT(cert-dcl58-cpp)
{
    static constexpr typename std::basic_format_parse_context<charT>::const_iterator
    parse(std::basic_format_parse_context<charT>& ctx)
    {
        if (ctx.begin() == ctx.end()) return ctx.begin();
        if (*ctx.begin() == ::yk::format_traits<charT>::brace_close) return ctx.begin();
        throw std::format_error(
            "format_by only accepts empty format spec; use "
            "`make_format_spec` for full controls."
        );
    }

    template<class OutIt>
    static OutIt format(yk::detail::format_by_proxy<FormatSpec, Ts...> const& proxy, std::basic_format_context<OutIt, charT>& ctx)
    {
        return yk::visit(
            [&]<class T>(T const& alt) {
                return std::format_to(
                    ctx.out(),
                    std::invoke(proxy.fspec, std::in_place_type<T>),
                    alt
                );
            },
            proxy.v
        );
    }
};

template<class... Ts, class charT>
    requires (std::formattable<yk::unwrap_recursive_t<Ts>, charT> && ...)
struct formatter<yk::rvariant<Ts...>, charT>  // NOLINT(cert-dcl58-cpp)
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
    static OutIt format(yk::rvariant<Ts...> const& var, std::basic_format_context<OutIt, charT>& ctx)
    {
        return yk::visit(
            [&]<class T>(T const& alt) {
                return std::format_to(
                    ctx.out(),
                    ::yk::format_traits<charT>::template brace_full<T const&>,
                    alt
                );
            },
            var
        );
    }
};

}  // namespace std

#endif
