#ifndef YK_INDIRECT_HPP
#define YK_INDIRECT_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/core/library.hpp>
#include <yk/core/type_traits.hpp>
#include <yk/core/hash.hpp>

#include <compare>
#include <memory>
#include <type_traits>
#include <utility>

#include <cassert>

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
        : alloc_(a)
        , ptr_(std::allocator_traits<Alloc>::allocate(alloc_, 1))
    {
        std::allocator_traits<Alloc>::construct(alloc_, get(), std::forward<Args>(args)...);
    }

    constexpr ~scoped_allocation()
        noexcept(noexcept(std::allocator_traits<Alloc>::deallocate(alloc_, ptr_, 1)))
    {
        if (ptr_) [[unlikely]] {
            std::allocator_traits<Alloc>::deallocate(alloc_, ptr_, 1);
        }
    }

    [[nodiscard]] constexpr Alloc get_allocator() const noexcept { return alloc_; }
    [[nodiscard]] constexpr auto get() const noexcept { return std::to_address(ptr_); }
    [[nodiscard]] constexpr pointer release() noexcept { return std::exchange(ptr_, nullptr); }

private:
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

    constexpr ~indirect() noexcept
    {
        if (ptr_) [[likely]] {
            destroy_deallocate();
        }
    }

    constexpr indirect& operator=(indirect const& other)
    {
        static_assert(std::is_copy_assignable_v<T>);
        static_assert(std::is_copy_constructible_v<T>);

        if (std::addressof(other) == this) [[unlikely]] {
            return *this;
        }

        constexpr bool pocca = std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value;

        if (other.ptr_) [[likely]] {
            if constexpr (std::allocator_traits<Allocator>::is_always_equal::value) {  // NOLINT(bugprone-branch-clone)
                if (ptr_) [[likely]] {
                    // both contain value and allocator is equal; copy assign
                    **this = *other;
                    if constexpr (pocca) {
                        alloc_ = other.alloc_;
                    }
                    return *this;
                }

                assert(other.ptr_ && !ptr_);
                if constexpr (pocca) {
                    ptr_ = other.make_obj(*other);
                    alloc_ = other.alloc_;
                } else {
                    ptr_ = this->make_obj(*other);
                }
                return *this;

            } else if (alloc_ == other.alloc_) {
                if (ptr_) [[likely]] {
                    // both contain value and allocator is equal; copy assign
                    **this = *other;
                    if constexpr (pocca) {
                        alloc_ = other.alloc_;
                    }
                    return *this;
                }

                assert(other.ptr_ && !ptr_);
                if constexpr (pocca) {
                    ptr_ = other.make_obj(*other);
                    alloc_ = other.alloc_;
                } else {
                    ptr_ = this->make_obj(*other);
                }
                return *this;

            } else {
                // other.ptr_ && (ptr_ || !ptr_)
                // either allocator is not equal or `this` does not contain value; create copy from `other`
                if (ptr_) [[likely]] {
                    destroy_deallocate();
                    ptr_ = nullptr; // make it safer
                }
                if constexpr (pocca) {
                    ptr_ = other.make_obj(*other);
                    alloc_ = other.alloc_;
                } else {
                    ptr_ = this->make_obj(*other);
                }
                return *this;
            }

        } else [[unlikely]] { // !other.ptr_
            if (ptr_) [[likely]] {
                destroy_deallocate();
                ptr_ = nullptr; // make it safer
            }
            if constexpr (pocca) {
                alloc_ = other.alloc_;
            }
            return *this;
        }
    }

    constexpr indirect& operator=(indirect&& other)
        noexcept(
            std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
            std::allocator_traits<Allocator>::is_always_equal::value
        )
    {
        static_assert(std::is_move_constructible_v<T>);

        if (std::addressof(other) == this) [[unlikely]] {
            return *this;
        }

        constexpr bool pocma = std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value;

        if (other.ptr_) [[likely]] {
            if constexpr (std::allocator_traits<Allocator>::is_always_equal::value) {
                if (ptr_) [[likely]] {
                    destroy_deallocate();
                }
                ptr_ = std::exchange(other.ptr_, nullptr);
                if constexpr (pocma) {
                    alloc_ = other.alloc_;
                }
                return *this;

            } else if (alloc_ == other.alloc_) {
                if (ptr_) [[likely]] {
                    destroy_deallocate();
                }
                ptr_ = std::exchange(other.ptr_, nullptr);
                if constexpr (pocma) {
                    alloc_ = other.alloc_;
                }
                return *this;

            } else {
                if (ptr_) [[likely]] {
                    destroy_deallocate();
                }
                if constexpr (pocma) {
                    ptr_ = other.make_obj(std::move(*other));
                } else {
                    ptr_ = this->make_obj(std::move(*other));
                }
                other.destroy_deallocate();
                return *this;
            }
        } else [[unlikely]] { // !other.ptr_
            if (ptr_) [[likely]] {
                destroy_deallocate();
                ptr_ = nullptr;
            }
            if constexpr (pocma) {
                alloc_ = other.alloc_;
            }
            return *this;
        }
    }

    template<class U = T>
        requires
            (!std::is_same_v<std::remove_cvref_t<U>, indirect>) &&
            std::is_constructible_v<T, U> &&
            std::is_assignable_v<T&, U>
    constexpr indirect& operator=(U&& u)
    {
        if (ptr_) [[likely]] {
            **this = std::forward<U>(u);

        } else [[unlikely]] {
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
    constexpr void destroy_deallocate()
    {
        assert(ptr_);
        std::allocator_traits<Allocator>::destroy(alloc_, std::to_address(ptr_));
        std::allocator_traits<Allocator>::deallocate(alloc_, ptr_, 1);
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
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) [[unlikely]] {
        return lhs.valueless_after_move() == rhs.valueless_after_move();
    } else [[likely]] {
        return *lhs == *rhs;
    }
}

template<class T, class Allocator, class U, class AA>
constexpr auto operator<=>(indirect<T, Allocator> const& lhs, indirect<U, AA> const& rhs)
    noexcept(core::synth_three_way_noexcept<T, U>) -> core::synth_three_way_result_t<T, U>
{
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) [[unlikely]] {
        return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
    } else [[likely]] {
        return core::synth_three_way(*lhs, *rhs);
    }
}

template<class T, class Allocator, class U>
constexpr bool operator==(indirect<T, Allocator> const& lhs, U const& rhs)
    noexcept(noexcept(*lhs == rhs))
{
    if (lhs.valueless_after_move()) [[unlikely]] {
        return false;
    } else [[likely]] {
        return *lhs == rhs;
    }
}

template<class T, class Allocator, class U> requires (!core::is_ttp_specialization_of_v<U, indirect>)
constexpr auto operator<=>(indirect<T, Allocator> const& lhs, U const& rhs)
    noexcept(core::synth_three_way_noexcept<T, U>) -> core::synth_three_way_result_t<T, U>
{
    if (lhs.valueless_after_move()) [[unlikely]] {
        return std::strong_ordering::less;
    } else [[likely]] {
        return core::synth_three_way(*lhs, rhs);
    }
}

}  // yk


namespace std {

template<class T, class Allocator>
    requires ::yk::core::is_hash_enabled_v<T>
struct hash<::yk::indirect<T, Allocator>>
{
    [[nodiscard]] static size_t operator()(::yk::indirect<T, Allocator> const& obj)
        noexcept(::yk::core::is_nothrow_hashable_v<T>)
    {
        if (obj.valueless_after_move()) [[unlikely]] {
            return 0xbaddeadbeefuz;
        } else [[likely]] {
            return std::hash<T>{}(*obj);
        }
    }
};

} // std


namespace yk {

template<class T, class Allocator>
    requires core::is_hash_enabled_v<T>
[[nodiscard]] std::size_t hash_value(indirect<T, Allocator> const& obj)
    noexcept(core::is_nothrow_hashable_v<T>)
{
    return std::hash<indirect<T, Allocator>>{}(obj);
}

} // yk

#endif  // YK_INDIRECT_HPP
