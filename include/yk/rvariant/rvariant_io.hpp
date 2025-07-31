#ifndef YK_RVARIANT_RVARIANT_IO_HPP
#define YK_RVARIANT_RVARIANT_IO_HPP

#include <yk/rvariant/rvariant.hpp>
#include <yk/core/io.hpp>

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

namespace detail {

template <class FormatSpec, class... Ts>
struct format_by_proxy {
    FormatSpec fs;
    yk::rvariant<Ts...> const& var;
};

}  // namespace detail

template <class... Fs>
struct format_spec : Fs... {
    using Fs::operator()...;
};

template <class... Ts>
consteval auto make_format_spec(std::format_string<Ts const&>... fmts)
{
    return format_spec{[=](std::in_place_type_t<Ts>) { return fmts; }...};
}

template <class Variant, class... Fmts>
consteval auto make_format_spec_for(Fmts const&... fmts)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) consteval {
        return make_format_spec<yk::variant_alternative_t<Is, Variant>...>(fmts...);
    }(std::make_index_sequence<yk::variant_size_v<Variant>>{});
}

template <class FormatSpec, class... Ts>
detail::format_by_proxy<FormatSpec, Ts...> format_by(FormatSpec fs, yk::rvariant<Ts...> const& var)
{
    return detail::format_by_proxy<FormatSpec, Ts...>{fs, var};
}

}  // namespace yk

namespace std {

template <class F, class... Ts>
struct formatter<yk::detail::format_by_proxy<F, Ts...>> {
    constexpr auto parse(std::format_parse_context& pc)
    {
        if (pc.begin() == pc.end()) return pc.begin();
        if (*pc.begin() == '}') return pc.begin();
        throw std::format_error("format_by only suppors empty specification");
    }

    template <class OutIt>
    auto format(yk::detail::format_by_proxy<F, Ts...> const& proxy, std::basic_format_context<OutIt, char>& fc) const
    {
        return yk::visit(
            [&]<class T>(T const& alt) { return std::vformat_to(fc.out(), std::invoke(proxy.fs, std::in_place_type<T>).get(), std::make_format_args(alt)); },
            proxy.var
        );
    }
};

template <class... Ts>
    requires (std::formattable<yk::unwrap_recursive_t<Ts>, char> && ...)
struct formatter<yk::rvariant<Ts...>> {
    constexpr auto parse(std::format_parse_context& pc)
    {
        if (pc.begin() == pc.end()) return pc.begin();
        if (*pc.begin() == '}') return pc.begin();
        throw std::format_error("rvariant only accepts empty specification");
      }

    template <class OutIt>
    auto format(yk::rvariant<Ts...> const& var, std::basic_format_context<OutIt, char>& fc) const
    {
        return yk::visit([&]<class T>(T const& alt) { return std::format_to(fc.out(), "{}", alt); }, var);
    }
};

}  // namespace std

#endif
