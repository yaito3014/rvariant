#ifndef YK_INDIRECT_HPP
#define YK_INDIRECT_HPP

#include <yk/core/library.hpp>
#include <yk/core/type_traits.hpp>

#include <compare>
#include <memory>
#include <utility>

namespace yk {

namespace detail {

template<class Alloc>
class scoped_allocation
{
public:
    using pointer = typename std::allocator_traits<Alloc>::pointer;
    using const_pointer = typename std::allocator_traits<Alloc>::const_pointer;


    template<class... Args>
    constexpr explicit scoped_allocation(Alloc const& a, std::in_place_t, Args&&... args)
        : scoped_allocation(a)
    {
        std::allocator_traits<Alloc>::construct(alloc_, get(), std::forward<Args>(args)...);
    }

    constexpr ~scoped_allocation()
        noexcept(noexcept(std::allocator_traits<Alloc>::deallocate(alloc_, ptr_, 1)))
    {
        if (ptr_) std::allocator_traits<Alloc>::deallocate(alloc_, ptr_, 1);
    }

    [[nodiscard]] constexpr Alloc get_allocator() const noexcept { return alloc_; }
    [[nodiscard]] constexpr auto get() const noexcept { return std::to_address(ptr_); }
    [[nodiscard]] constexpr pointer release() noexcept { return std::exchange(ptr_, nullptr); }

private:
    constexpr explicit scoped_allocation(Alloc const& a)
        : alloc_(a)
        , ptr_(std::allocator_traits<Alloc>::allocate(alloc_, 1))
    {}

    YK_NO_UNIQUE_ADDRESS Alloc alloc_;
    pointer ptr_;
};

}  // detail


// Polyfill for the C++26 `std::indirect`
// https://eel.is/c++draft/indirect
template<class T, class Allocator = std::allocator<T>>
class indirect
{
    static_assert(std::is_object_v<T>);
    static_assert(!std::is_array_v<T>);
    static_assert(!std::is_same_v<T, std::in_place_t>);
    static_assert(!core::is_ttp_specialization_of_v<T, std::in_place_type_t>);
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(std::is_same_v<T, typename std::allocator_traits<Allocator>::value_type>);

public:
    using value_type = T;
    using allocator_type = Allocator;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    constexpr explicit indirect() requires std::is_default_constructible_v<Allocator>
        : ptr_(make_obj())
    {}

    constexpr indirect(indirect const& other)
        : indirect(
            std::allocator_arg,
            std::allocator_traits<Allocator>::select_on_container_copy_construction(other.alloc_),
            other
        )
    {}

    constexpr explicit indirect(std::allocator_arg_t, Allocator const& a)
        : alloc_(a)
        , ptr_(make_obj())
    {}

    constexpr indirect(std::allocator_arg_t, Allocator const& a, indirect const& other)
        : alloc_(a)
        , ptr_(other.ptr_ ? make_obj(std::as_const(*other.ptr_)) : nullptr)
    {
        static_assert(std::is_copy_constructible_v<T>);
    }

    constexpr indirect(indirect&& other) noexcept
        : alloc_(std::move(other.alloc_))
        , ptr_(std::exchange(other.ptr_, nullptr))
    {}

    constexpr indirect(std::allocator_arg_t, Allocator const& a, indirect&& other)
        noexcept(std::allocator_traits<Allocator>::is_always_equal::value)
        : alloc_(a)
        , ptr_(alloc_ == other.alloc_
            ? std::exchange(other.ptr_, nullptr)
            : make_obj(*std::move(other))
        )
    {}

    template<class U = T>
        requires
            (!std::is_same_v<std::remove_cvref_t<U>, indirect>) &&
            (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>) &&
            std::is_constructible_v<T, U> &&
            std::is_default_constructible_v<Allocator>
    constexpr explicit indirect(U&& u)
        : ptr_(make_obj(std::forward<U>(u)))
    {}

    template<class U = T>
        requires
            (!std::is_same_v<std::remove_cvref_t<U>, indirect>) &&
            (!std::is_same_v<std::remove_cvref_t<U>, std::in_place_t>) &&
            std::is_constructible_v<T, U>
    constexpr explicit indirect(std::allocator_arg_t, Allocator const& a, U&& u)
        : alloc_(a)
        , ptr_(make_obj(std::forward<U>(u)))
    {}

    template<class... Us>
        requires
            std::is_constructible_v<T, Us...> &&
            std::is_default_constructible_v<Allocator>
    constexpr explicit indirect(std::in_place_t, Us&&... us)
        : ptr_(make_obj(std::forward<Us>(us)...))
    {}

    template<class... Us>
        requires
            std::is_constructible_v<T, Us...>
    constexpr explicit indirect(std::allocator_arg_t, Allocator const& a, std::in_place_t, Us&&... us)
        : alloc_(a)
        , ptr_(make_obj(std::forward<Us>(us)...))
    {}

    template<class I, class... Us>
        requires
            std::is_constructible_v<T, std::initializer_list<I>&, Us...> &&
            std::is_default_constructible_v<Allocator>
    constexpr explicit indirect(std::in_place_t, std::initializer_list<I> il, Us&&... us)
        : ptr_(make_obj(il, std::forward<Us>(us)...))
    {}

    template<class I, class... Us>
        requires
            std::is_constructible_v<T, std::initializer_list<I>&, Us...>
    constexpr explicit indirect(std::allocator_arg_t, Allocator const& a, std::in_place_t, std::initializer_list<I> il, Us&&... us)
        : alloc_(a)
        , ptr_(make_obj(il, std::forward<Us>(us)...))
    {}

    constexpr ~indirect() { reset(nullptr); }

    constexpr indirect& operator=(indirect const& other)
    {
        static_assert(std::is_copy_assignable_v<T>);
        static_assert(std::is_copy_constructible_v<T>);

        if (std::addressof(other) == this) return *this;

        constexpr bool pocca = std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value;

        pointer p = nullptr;
        if (other.ptr_) {
            if constexpr (std::allocator_traits<Allocator>::is_always_equal::value) {
                if (ptr_) {
                    // both contain value and allocator is equal; copy assign
                    **this = *other;
                    if constexpr (pocca) {
                        alloc_ = other.alloc_;
                    }
                    return *this;
                }
            } else if (alloc_ == other.alloc_) {
                if (ptr_) {
                    // both contain value and allocator is equal; copy assign
                    **this = *other;
                    if constexpr (pocca) {
                        alloc_ = other.alloc_;
                    }
                    return *this;
                }
            }
            // either allocator is not equal or `this` does not contain value; create copy from `other`
            indirect const& x = pocca ? other : *this;
            p = x.make_obj(*other);
        }

        // destroy current contained value, optionally replace with new one
        reset(p);
        if constexpr (pocca) {
            alloc_ = other.alloc_;
        }
        return *this;
    }

    constexpr indirect& operator=(indirect&& other)
        noexcept(
            std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
            std::allocator_traits<Allocator>::is_always_equal::value
        )
    {
        static_assert(std::is_move_constructible_v<T>);

        if (std::addressof(other) == this) return *this;

        constexpr bool pocma = std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value;

        pointer p = nullptr;
        if (other.ptr_) {
            if constexpr (std::allocator_traits<Allocator>::is_always_equal::value) {
                p = std::exchange(other.ptr_, nullptr);
            } else if (alloc_ == other.alloc_) {
                p = std::exchange(other.ptr_, nullptr);
            } else {
                indirect& x = pocma ? other : *this;
                p = x.make_obj(std::move(*other));
            }
        }

        reset(p);
        if constexpr (pocma) {
            alloc_ = other.alloc_;
        }
        other.reset(nullptr);  // TODO: investigate whether this is needed
        return *this;
    }

    template<class U = T>
        requires
            (!std::is_same_v<std::remove_cvref_t<U>, indirect>) &&
            std::is_constructible_v<T, U> &&
            std::is_assignable_v<T&, U>
    constexpr indirect& operator=(U&& u)
    {
        if (ptr_) {
            **this = std::forward<U>(u);

        } else {
            ptr_ = make_obj(std::forward<U>(u));
        }
        return *this;
    }

    [[nodiscard]] constexpr T& operator*() & noexcept YK_LIFETIMEBOUND { return *ptr_; }
    [[nodiscard]] constexpr T const& operator*() const& noexcept YK_LIFETIMEBOUND { return *ptr_; }
    [[nodiscard]] constexpr T&& operator*() && noexcept YK_LIFETIMEBOUND { return std::move(*ptr_); }
    [[nodiscard]] constexpr T const&& operator*() const&& noexcept YK_LIFETIMEBOUND { return std::move(*ptr_); }

    [[nodiscard]] constexpr pointer operator->() noexcept YK_LIFETIMEBOUND { return ptr_; }
    [[nodiscard]] constexpr const_pointer operator->() const noexcept YK_LIFETIMEBOUND { return ptr_; }

    [[nodiscard]] constexpr bool valueless_after_move() const noexcept { return ptr_ == nullptr; }

    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return alloc_; }

    constexpr void swap(indirect& other)
        noexcept(
            std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
            std::allocator_traits<Allocator>::is_always_equal::value
        )
    {
        using std::swap;
        swap(ptr_, other.ptr_);

        if constexpr (std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
            swap(alloc_, other.alloc_);
        }
    }

    friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        return lhs.swap(rhs);
    }

private:
    constexpr pointer reset(pointer p)
    {
        if (ptr_) {
            std::allocator_traits<Allocator>::destroy(alloc_, ptr_);
            std::allocator_traits<Allocator>::deallocate(alloc_, ptr_, 1);
        }
        return ptr_ = p;
    }

    template<class... Args>
    [[nodiscard]] constexpr T* make_obj(Args&&... args) const
    {
        detail::scoped_allocation sa(alloc_, std::in_place, std::forward<Args>(args)...);
        return sa.release();
    }

    YK_NO_UNIQUE_ADDRESS Allocator alloc_ = Allocator();
    pointer ptr_;
};

template<class Value>
indirect(Value)
    -> indirect<Value>;

template<class Value, class Allocator>
indirect(std::allocator_arg_t, Allocator, Value)
    -> indirect<Value, typename std::allocator_traits<Allocator>::template rebind_alloc<Value>>;


template<class T, class Allocator, class U, class AA>
constexpr bool operator==(indirect<T, Allocator> const& lhs, indirect<U, AA> const& rhs)
    noexcept(noexcept(*lhs == *rhs))
{
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
        return lhs.valueless_after_move() == rhs.valueless_after_move();
    }
    return *lhs == *rhs;
}

template<class T, class Allocator, class U, class AA>
constexpr auto operator<=>(indirect<T, Allocator> const& lhs, indirect<U, AA> const& rhs)
    noexcept(core::synth_three_way_noexcept<T, U>) -> core::synth_three_way_result_t<T, U>
{
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
        return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
    }
    return core::synth_three_way(*lhs, *rhs);
}

template<class T, class Allocator, class U>
constexpr bool operator==(indirect<T, Allocator> const& lhs, U const& rhs)
    noexcept(noexcept(*lhs == rhs))
{
    if (lhs.valueless_after_move()) return false;
    return *lhs == rhs;
}

template<class T, class Allocator, class U> requires (!core::is_ttp_specialization_of_v<U, indirect>)
constexpr auto operator<=>(indirect<T, Allocator> const& lhs, U const& rhs)
    noexcept(core::synth_three_way_noexcept<T, U>) -> core::synth_three_way_result_t<T, U>
{
    if (lhs.valueless_after_move()) return std::strong_ordering::less;
    return core::synth_three_way(*lhs, rhs);
}

}  // yk


namespace std {

template<class T, class A>
    requires requires { typename hash<T>; }
struct hash<yk::indirect<T, A>>
{
    std::size_t operator()(yk::indirect<T, A> const& i) const
        noexcept(noexcept(hash<T>{}(*i))) /* strengthened */
    {
        if (i.valueless_after_move()) return 33 - 4zu;  // arbitrary number
        return hash<T>{}(*i);
    }
};

}  // std

#endif  // YK_INDIRECT_HPP
