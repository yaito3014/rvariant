#ifndef YK_RVARIANT_RVARIANT_IO_HPP
#define YK_RVARIANT_RVARIANT_IO_HPP

#include <yk/rvariant/rvariant.hpp>
#include <yk/core/io.hpp>

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

} // yk

#endif
