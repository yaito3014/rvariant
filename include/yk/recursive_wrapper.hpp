#ifndef YK_RECURSIVE_WRAPPER_HPP
#define YK_RECURSIVE_WRAPPER_HPP

#include <compare>
#include <initializer_list>
#include <memory>
#include <utility>

namespace yk {

// TODO: replace std::indirect with yk::indirect
template<class T, class Allocator = std::allocator<T>>
class recursive_wrapper : private std::indirect<T, Allocator> {
    using base_type = std::indirect<T, Allocator>;

public:
    using base_type::allocator_type;
    using base_type::const_pointer;
    using base_type::pointer;
    using base_type::value_type;

    using base_type::base_type;

    template<class U = T>
        requires requires {
            requires (!std::is_same_v<std::remove_cvref_t<U>, recursive_wrapper>);
            requires (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>);
            requires std::is_constructible_v<T, U>;
            requires std::is_default_constructible_v<Allocator>;
        }
    constexpr explicit recursive_wrapper(U&& x) : base_type(std::forward<U>(x)) {}

    template<class U = T>
        requires requires {
            requires (!std::is_same_v<std::remove_cvref_t<U>, recursive_wrapper>);
            requires (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>);
            requires std::is_constructible_v<T, U>;
        }
    constexpr explicit recursive_wrapper(std::allocator_arg_t, Allocator const& a, U&& x) : base_type(std::allocator_arg, a, std::forward<U>(x)) {}

    template<class... Us>
        requires requires {
            requires std::is_constructible_v<T, Us...>;
            requires std::is_default_constructible_v<Allocator>;
        }
    constexpr explicit recursive_wrapper(std::in_place_t, Us&&... us) : base_type(std::in_place, std::forward<Us>(us)...) {}

    template<class... Us>
        requires std::is_constructible_v<T, Us...>
    constexpr explicit recursive_wrapper(std::allocator_arg_t, Allocator const& a, std::in_place_t, Us&&... us)
        : base_type(std::allocator_arg, a, std::in_place, std::forward<Us>(us)...) {}

    template<class I, class... Us>
        requires requires {
            requires std::is_constructible_v<T, std::initializer_list<I>&, Us...>;
            requires std::is_default_constructible_v<Allocator>;
        }
    constexpr explicit recursive_wrapper(std::in_place_t, std::initializer_list<I> il, Us&&... us) : base_type(std::in_place, il, std::forward<Us>(us)...) {}

    template<class I, class... Us>
        requires std::is_constructible_v<T, std::initializer_list<I>&, Us...>
    constexpr explicit recursive_wrapper(std::allocator_arg_t, Allocator const& a, std::in_place_t, std::initializer_list<I> il, Us&&... us)
        : base_type(std::allocator_arg, a, std::in_place, il, std::forward<Us>(us)...) {}

    using base_type::operator=;

    using base_type::operator*;
    using base_type::operator->;
    using base_type::get_allocator;
    using base_type::valueless_after_move;

    using base_type::swap;

    friend constexpr void swap(recursive_wrapper& lhs, recursive_wrapper& rhs) noexcept(noexcept(lhs.swap(rhs))) { lhs.swap(rhs); }

    friend constexpr bool operator==(recursive_wrapper const& lhs, recursive_wrapper const& rhs) noexcept(noexcept(*lhs == *rhs)) {
        if (lhs.valuless_after_move() || rhs.valueless_after_move()) return lhs.valuless_after_move() == rhs.valuless_after_move();
        return *lhs == *rhs;
    }

    friend constexpr auto operator<=>(recursive_wrapper const& lhs, recursive_wrapper const& rhs) {
        if (lhs.valuless_after_move() || rhs.valueless_after_move()) return !lhs.valuless_after_move() <=> !rhs.valuless_after_move();
        return *lhs <=> *rhs;
    }

    template<class U>
    friend constexpr bool operator==(recursive_wrapper const& lhs, U const& rhs) noexcept(noexcept(*lhs == rhs)) {
        if (lhs.valueless_after_move()) return false;
        return *lhs == rhs;
    }

    template<class U>
    friend constexpr auto operator<=>(recursive_wrapper const& lhs, U const& rhs) {
        if (lhs.valueless_after_move()) return std::strong_ordering::less;
        return *lhs <=> rhs;
    }
};

template<class Value>
recursive_wrapper(Value) -> recursive_wrapper<Value>;

template<class Allocator, class Value>
recursive_wrapper(std::allocator_arg_t, Allocator, Value) -> recursive_wrapper<Value, typename std::allocator_traits<Allocator>::template rebind_alloc<Value>>;

}  // namespace yk

#endif  // YK_RECURSIVE_WRAPPER_HPP
