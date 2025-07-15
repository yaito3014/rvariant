#ifndef YK_RVARIANT_DETAIL_RVARIANT_FWD_HPP
#define YK_RVARIANT_DETAIL_RVARIANT_FWD_HPP




namespace yk {

template<class... Ts>
class rvariant;

template<class T, class Allocator>
class recursive_wrapper;


namespace detail {

struct valueless_t
{
    constexpr explicit valueless_t() = default;
};

inline constexpr valueless_t valueless{};

} // detail

} // yk

#endif
