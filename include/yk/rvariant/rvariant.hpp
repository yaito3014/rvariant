#ifndef YK_RVARIANT_RVARIANT_HPP
#define YK_RVARIANT_RVARIANT_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/detail/variant_requirements.hpp>
#include <yk/rvariant/detail/variant_storage.hpp>
#include <yk/rvariant/detail/recursive_traits.hpp>
#include <yk/rvariant/variant_helper.hpp>
#include <yk/rvariant/subset.hpp>

#include <yk/core/type_traits.hpp>
#include <yk/core/cond_trivial.hpp>

#include <yk/hash.hpp>

#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <variant>
#include <memory>

#include <cstddef>
#include <cassert>

namespace yk {

namespace detail {

template<class... Ts>
struct rvariant_base
{
    static constexpr bool need_destructor_call = !std::conjunction_v<std::is_trivially_destructible<Ts>...>;

public:
    using storage_type = make_variadic_union_t<Ts...>;
    static constexpr bool never_valueless = storage_type::never_valueless;

    // internal constructor; this is not the same as the most derived class' default constructor (which uses T0)
    constexpr rvariant_base() noexcept
        : storage_{} // valueless
    {}

YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
    // Primary constructor called from derived class
    template<std::size_t I, class... Args>
        requires std::is_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>
    constexpr explicit rvariant_base(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>)
        : storage_(std::in_place_index<I>, std::forward<Args>(args)...)
        , index_{static_cast<variant_index_t<sizeof...(Ts)>>(I)}
    {}
YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END

    // Primary constructor called from derived class
    constexpr explicit rvariant_base(valueless_t) noexcept
        : storage_{} // valueless
    {}

    // Copy constructor
    constexpr void _copy_construct(rvariant_base const& w)
        noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<Ts>...>)
    {
        w.raw_visit([this]<std::size_t j, class T>(std::in_place_index_t<j>, [[maybe_unused]] T const& alt)
            noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<Ts>...>)
        {
            if constexpr (j != std::variant_npos) {
                construct_on_valueless<j>(alt);
            } else {
                (void)this;
            }
        });
    }

    // Move constructor
    constexpr void _move_construct(rvariant_base&& w)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
    {
        std::move(w).raw_visit([this]<std::size_t j, class T>(std::in_place_index_t<j>, [[maybe_unused]] T&& alt)
            noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
        {
            if constexpr (j != std::variant_npos) {
                static_assert(std::is_rvalue_reference_v<T&&>);
                construct_on_valueless<j>(std::move(alt)); // NOLINT(bugprone-move-forwarding-reference)
            } else {
                (void)this;
            }
        });
    }

    // Copy assignment
    constexpr void _copy_assign(rvariant_base const& rhs)
        noexcept(std::conjunction_v<variant_nothrow_copy_assignable<Ts>...>)
    {
    YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
        rhs.raw_visit([this]<std::size_t j, class T>(std::in_place_index_t<j>, T const& rhs_alt)
            noexcept(std::conjunction_v<variant_nothrow_copy_assignable<Ts>...>)
        {
            if constexpr (j == std::variant_npos) {
                (void)rhs_alt;
                visit_reset();
            } else {
                if (index_ == j) {
                    raw_get<j>(storage()) = rhs_alt;
                } else {
                    // CC(noexcept) && MC(throw)    => A
                    // CC(noexcept) && MC(noexcept) => A
                    // CC(throw)    && MC(throw)    => A
                    // CC(throw)    && MC(noexcept) => B
                    if constexpr (std::is_nothrow_copy_constructible_v<T> || !std::is_nothrow_move_constructible_v<T>) {
                        reset_construct<j>(rhs_alt);  // A
                    } else {
                        auto tmp = rhs_alt;
                        reset_construct<j>(std::move(tmp)); // B
                    }
                }
            }
        });
    YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END
    }

    // Move assignment
    constexpr void _move_assign(rvariant_base&& rhs)
        noexcept(std::conjunction_v<variant_nothrow_move_assignable<Ts>...>)
    {
        std::move(rhs).raw_visit([this]<std::size_t j, class T>(std::in_place_index_t<j>, [[maybe_unused]] T&& rhs_alt)
            noexcept(std::conjunction_v<variant_nothrow_move_assignable<Ts>...>)
        {
            if constexpr (j == std::variant_npos) {
                visit_reset();
            } else {
                static_assert(std::is_rvalue_reference_v<T&&>);
                if (index_ == j) {
                    raw_get<j>(storage()) = std::move(rhs_alt); // NOLINT(bugprone-move-forwarding-reference)
                } else {
                    reset_construct<j>(std::move(rhs_alt)); // NOLINT(bugprone-move-forwarding-reference)
                }
            }
        });
    }

    // -------------------------------------------------------

    [[nodiscard]] constexpr bool valueless_by_exception() const noexcept
    {
        if constexpr (never_valueless) {
            assert(index_ != detail::variant_npos<sizeof...(Ts)>);
            return false;
        } else {
            return index_ == detail::variant_npos<sizeof...(Ts)>;
        }
    }
    [[nodiscard]] constexpr std::size_t index() const noexcept { return static_cast<std::size_t>(index_); }

    // internal
    template<std::size_t I>
    constexpr void destroy() noexcept
    {
        if constexpr (need_destructor_call) {
            // ReSharper disable once CppTypeAliasNeverUsed
            using T = core::pack_indexing_t<I, Ts...>;
            auto&& alt = raw_get<I>(storage_);
            alt.~T();
        }
    }

    // internal
    constexpr void visit_destroy() noexcept
    {
        if constexpr (need_destructor_call) {
            this->raw_visit([]<std::size_t i, class T>(std::in_place_index_t<i>, [[maybe_unused]] T& alt) static noexcept {
                if constexpr (i != std::variant_npos) {
                    alt.~T();
                }
            });
        }
    }

    // internal
    constexpr void visit_reset() noexcept
    {
        if constexpr (need_destructor_call) {
            this->raw_visit([this]<std::size_t i, class T>(std::in_place_index_t<i>, [[maybe_unused]] T& alt) noexcept {
                if constexpr (i != std::variant_npos) {
                    alt.~T();
                    index_ = variant_npos<sizeof...(Ts)>;
                }
            });
        } else {
            index_ = variant_npos<sizeof...(Ts)>;
        }
    }
    // internal
    template<std::size_t I>
    constexpr void reset() noexcept
    {
        assert(index_ == I);
        if constexpr (I != std::variant_npos) {
            // ReSharper disable once CppTypeAliasNeverUsed
            using T = core::pack_indexing_t<I, Ts...>;
            auto&& alt = raw_get<I>(storage_);
            alt.~T();
            index_ = variant_npos<sizeof...(Ts)>;
        }
    }

YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
    template<std::size_t I, class... Args>
    constexpr void construct_on_valueless(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>)
    {
        static_assert(I != std::variant_npos);
        assert(index_ == variant_npos<sizeof...(Ts)>);
        std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...);
        index_ = static_cast<variant_index_t<sizeof...(Ts)>>(I);
    }

    template<std::size_t I, class... Args>
    constexpr void reset_construct(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>)
    {
        static_assert(I != std::variant_npos);
        visit_reset();
        std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...);
        index_ = static_cast<variant_index_t<sizeof...(Ts)>>(I);
    }

    template<std::size_t i, std::size_t j, class... Args>
    constexpr void reset_construct(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<j, Ts...>, Args...>)
    {
        if constexpr (i != std::variant_npos) {
            destroy<i>();
            if constexpr (!std::is_nothrow_constructible_v<core::pack_indexing_t<j, Ts...>, Args...>) {
                index_ = variant_npos<sizeof...(Ts)>;
            }
        }
        static_assert(j != std::variant_npos);
        std::construct_at(&storage_, std::in_place_index<j>, std::forward<Args>(args)...);
        index_ = static_cast<variant_index_t<sizeof...(Ts)>>(j);
    }

    template<std::size_t I, class... Args>
    constexpr void reset_construct_never_valueless(Args&&... args) noexcept
    {
        static_assert(I != std::variant_npos);
        static_assert(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>);
        visit_destroy();
        static_assert(noexcept(std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...)));
        std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...);
        index_ = static_cast<variant_index_t<sizeof...(Ts)>>(I);
    }

YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END

    // used in swap operation
    constexpr void reset_steal_from(rvariant_base&& rhs)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
    {
        visit_reset();

        std::move(rhs).raw_visit([this]<std::size_t i, class T>(std::in_place_index_t<i>, [[maybe_unused]] T&& alt)
            noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
        {
            if constexpr (i != std::variant_npos) {
                static_assert(std::is_rvalue_reference_v<T&&>);
                this->template construct_on_valueless<i>(std::move(alt)); // NOLINT(bugprone-move-forwarding-reference)
            }
        });
    }

    [[nodiscard]] constexpr storage_type &       storage() &       noexcept { return storage_; }
    [[nodiscard]] constexpr storage_type const&  storage() const&  noexcept { return storage_; }
    [[nodiscard]] constexpr storage_type &&      storage() &&      noexcept { return std::move(storage_); }
    [[nodiscard]] constexpr storage_type const&& storage() const&& noexcept { return std::move(storage_); }

    template<class Self, class Visitor>
    constexpr auto
    raw_visit(this Self&& self, Visitor&& vis) noexcept(raw_visit_noexcept<Visitor, decltype(std::forward<Self>(self).storage())>)
        -> raw_visit_return_type<Visitor, decltype(std::forward<Self>(self).storage())>
    {
        using Storage = decltype(std::forward<Self>(self).storage());
        constexpr auto const& table = raw_visit_dispatch_table<Visitor&&, Storage>::value;
        auto&& f = table[valueless_bias<storage_type::never_valueless>(self.index_)];
        return std::invoke(f, std::forward<Visitor>(vis), std::forward<Self>(self).storage());
    }

    storage_type storage_{}; // valueless
    variant_index_t<sizeof...(Ts)> index_ = variant_npos<sizeof...(Ts)>;
};


template<class... Ts>
struct rvariant_non_trivial_destructor : rvariant_base<Ts...>
{
    constexpr ~rvariant_non_trivial_destructor() noexcept
    {
        rvariant_non_trivial_destructor::rvariant_base::visit_destroy();
    }

    using rvariant_non_trivial_destructor::rvariant_base::rvariant_base;
    rvariant_non_trivial_destructor() = default;
    rvariant_non_trivial_destructor(rvariant_non_trivial_destructor const&) = default;
    rvariant_non_trivial_destructor(rvariant_non_trivial_destructor&&) = default;
    rvariant_non_trivial_destructor& operator=(rvariant_non_trivial_destructor const&) = default;
    rvariant_non_trivial_destructor& operator=(rvariant_non_trivial_destructor&&) = default;
};

template<class... Ts>
using rvariant_destructor_base_t = std::conditional_t<
    std::conjunction_v<std::is_trivially_destructible<Ts>...>,
    rvariant_base<Ts...>,
    rvariant_non_trivial_destructor<Ts...>
>;

template<class... Ts>
using rvariant_base_t = core::cond_trivial<rvariant_destructor_base_t<Ts...>, Ts...>;

template<class Compare, class... Ts>
[[nodiscard]] constexpr bool compare_relops(rvariant<Ts...> const& v, rvariant<Ts...> const& w) noexcept(std::conjunction_v<std::is_nothrow_invocable<Compare, Ts const&, Ts const&>...>)
{
    return v.raw_visit([&w]<std::size_t i, class T>(std::in_place_index_t<i>, [[maybe_unused]] T const& alt)
        noexcept(std::conjunction_v<std::is_nothrow_invocable<Compare, Ts const&, Ts const&>...>)
    {
        if constexpr (i != std::variant_npos) {
            return Compare{}(alt, detail::raw_get<i>(w.storage()));
        } else {
            return Compare{}(0, 0);
        }
    });
}

}  // detail


template<class... Ts>
class rvariant : private detail::rvariant_base_t<Ts...>
{
    static_assert(std::conjunction_v<detail::check_recursive_wrapper_duplicate<Ts, Ts...>...>);
    static_assert((core::Cpp17Destructible<Ts> && ...), "All types shall meet the Cpp17Destructible requirements ([variant.variant.general]).");
    static_assert(sizeof...(Ts) > 0, "A variant with no template arguments shall not be instantiated ([variant.variant.general]).");

    using unwrapped_types = core::type_list<unwrap_recursive_t<Ts>...>;

    using base_type = detail::rvariant_base_t<Ts...>;
    friend struct detail::rvariant_base<Ts...>;

    using base_type::storage;
    using base_type::index_;
    using base_type::raw_visit;
    using base_type::visit_reset;
    using base_type::reset;

public:
    using base_type::never_valueless;
    using base_type::valueless_by_exception;
    using base_type::index;

    // Default constructor
    constexpr rvariant() noexcept(std::is_nothrow_default_constructible_v<core::pack_indexing_t<0, Ts...>>)
        requires std::is_default_constructible_v<core::pack_indexing_t<0, Ts...>>
        : base_type(std::in_place_index<0>) // value-initialized
    {}

    ~rvariant() noexcept = default;
    rvariant(rvariant const&) = default;
    rvariant(rvariant&&) = default;
    rvariant& operator=(rvariant const&) = default;
    rvariant& operator=(rvariant&&) = default;

    // --------------------------------------

    // Generic constructor
    // <https://eel.is/c++draft/variant.ctor#lib:variant,constructor___>
    template<class T>
        requires
            (sizeof...(Ts) > 0) &&
            (!std::is_same_v<std::remove_cvref_t<T>, rvariant>) &&
            (!core::is_ttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_type_t>) &&
            (!core::is_nttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_index_t>) &&
            std::is_constructible_v<core::aggregate_initialize_resolution_t<T, Ts...>, T>
    constexpr /* not explicit */ rvariant(T&& t)
        noexcept(std::is_nothrow_constructible_v<core::aggregate_initialize_resolution_t<T, Ts...>, T>)
        : base_type(std::in_place_index<core::aggregate_initialize_resolution_index<T, Ts...>>, std::forward<T>(t))
    {}

    // Generic assignment operator
    // <https://eel.is/c++draft/variant.assign#lib:operator=,variant__>
    template<class T>
        requires
            (!std::is_same_v<std::remove_cvref_t<T>, rvariant>) &&
            detail::variant_assignable<core::aggregate_initialize_resolution_t<T, Ts...>, T>::value
    constexpr rvariant& operator=(T&& t)
        noexcept(detail::variant_nothrow_assignable<core::aggregate_initialize_resolution_t<T, Ts...>, T>::value)
    {
        using Tj = core::aggregate_initialize_resolution_t<T, Ts...>; // either plain type or wrapped with recursive_wrapper
        constexpr std::size_t j = core::aggregate_initialize_resolution_index<T, Ts...>;
        static_assert(j != std::variant_npos);

    YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
        this->raw_visit([this, &t]<std::size_t i, class Ti>(std::in_place_index_t<i>, [[maybe_unused]] Ti& ti)
            noexcept(detail::variant_nothrow_assignable<Tj, T>::value)
        {
            if constexpr (i == j) {
                ti = std::forward<T>(t);
            } else {
                // TC(noexcept) && MC(throw)    => A maybe valueless if | never |
                // TC(noexcept) && MC(noexcept) => A maybe valueless if | never |
                // TC(throw)    && MC(throw)    => A maybe valueless if | TC throws => yes | MC throws => yes |
                // TC(throw)    && MC(noexcept) => B maybe valueless if | never |
                if constexpr (std::is_nothrow_constructible_v<Tj, T> || !std::is_nothrow_move_constructible_v<Tj>) {
                    // strengthen this branch to provide never-valueless guarantee on certain conditions
                    if constexpr (std::is_nothrow_constructible_v<Tj, T>) {
                        // TC(noexcept); never valueless
                        base_type::template reset_construct<i, j>(std::forward<T>(t));
                    } else {
                        // TC(throw) && MC(throw)
                        Tj tmp(std::forward<T>(t));
                        base_type::template reset_construct<i, j>(std::move(tmp)); // valueless IFF MC throws
                    }
                } else {
                    Tj tmp(std::forward<T>(t));
                    base_type::template reset_construct<i, j>(std::move(tmp)); // B
                }
            }
        });
        return *this;
    YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END
    }

    // --------------------------------------

    // Flexible copy constructor
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            (!std::disjunction_v<std::is_same<rvariant<Us...>, unwrap_recursive_t<Ts>>...>) &&
            std::conjunction_v<std::is_constructible<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us const&>...>
    constexpr rvariant(rvariant<Us...> const& w)
        noexcept(std::conjunction_v<std::is_nothrow_constructible<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us const&>...>)
    {
        w.raw_visit([this]<std::size_t j, class Uj>(std::in_place_index_t<j>, [[maybe_unused]] Uj const& uj)
            noexcept(std::conjunction_v<std::is_nothrow_constructible<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us const&>...>)
        {
            if constexpr (j != std::variant_npos) {
                using maybe_wrapped = detail::select_maybe_wrapped<unwrap_recursive_t<Uj>, Ts...>;
                using VT = typename maybe_wrapped::type;
                static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<Uj>>);
                base_type::template construct_on_valueless<maybe_wrapped::index>(detail::forward_maybe_wrapped<VT>(uj));
            }
        });
    }

    // Flexible move constructor
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            (!std::disjunction_v<std::is_same<rvariant<Us...>, unwrap_recursive_t<Ts>>...>) &&
            std::conjunction_v<std::is_constructible<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us&&>...>
    constexpr rvariant(rvariant<Us...>&& w)
        noexcept(std::conjunction_v<std::is_nothrow_constructible<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us&&>...>)
    {
        std::move(w).raw_visit([this]<std::size_t j, class Uj>(std::in_place_index_t<j>, [[maybe_unused]] Uj&& uj)
            noexcept(std::conjunction_v<std::is_nothrow_constructible<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us&&>...>)
        {
            if constexpr (j != std::variant_npos) {
                using maybe_wrapped = detail::select_maybe_wrapped<unwrap_recursive_t<Uj>, Ts...>;
                using VT = typename maybe_wrapped::type;
                static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<Uj>>);
                static_assert(std::is_rvalue_reference_v<Uj&&>);
                base_type::template construct_on_valueless<maybe_wrapped::index>(detail::forward_maybe_wrapped<VT>(std::move(uj))); // NOLINT(bugprone-move-forwarding-reference)
            }
        });
    }

    // --------------------------------------

    // Flexible copy assignment operator
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            (!std::disjunction_v<std::is_same<rvariant<Us...>, unwrap_recursive_t<Ts>>...>) &&
            std::conjunction_v<detail::variant_copy_assignable<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us const&>...>
    constexpr rvariant& operator=(rvariant<Us...> const& rhs)
        noexcept(std::conjunction_v<detail::variant_nothrow_copy_assignable<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us const&>...>)
    {
        rhs.raw_visit([this]<std::size_t j, class Uj>(std::in_place_index_t<j>, [[maybe_unused]] Uj const& uj)
            noexcept(std::conjunction_v<detail::variant_nothrow_copy_assignable<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us const&>...>)
        {
            if constexpr (j == std::variant_npos) {
                this->visit_reset();

            } else {
                using maybe_wrapped = detail::select_maybe_wrapped<unwrap_recursive_t<Uj>, Ts...>;
                using VT = typename maybe_wrapped::type;
                static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<Uj>>);

                this->raw_visit([this, &uj]<std::size_t i, class Ti>(std::in_place_index_t<i>, [[maybe_unused]] Ti& ti)
                    noexcept(std::conjunction_v<detail::variant_nothrow_copy_assignable<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us const&>...>)
                {
                    constexpr std::size_t VTi = maybe_wrapped::index;
                    if constexpr (i == std::variant_npos) { // this is valueless, rhs holds value
                        base_type::template construct_on_valueless<VTi>(detail::forward_maybe_wrapped<VT>(uj));

                    } else if constexpr (std::is_same_v<unwrap_recursive_t<Ti>, unwrap_recursive_t<Uj>>) {
                        ti = detail::forward_maybe_wrapped<Ti>(uj);

                    } else if constexpr (std::is_nothrow_constructible_v<VT, Uj const&> || !std::is_nothrow_move_constructible_v<VT>) {
                        base_type::template reset_construct<i, VTi>(detail::forward_maybe_wrapped<VT>(uj));

                    } else {
                        VT tmp = detail::forward_maybe_wrapped<VT>(uj); // may throw
                        base_type::template reset_construct<i, VTi>(std::move(tmp));
                    }
                });
            }
        });
        return *this;
    }

    // Flexible move assignment operator
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            (!std::disjunction_v<std::is_same<rvariant<Us...>, unwrap_recursive_t<Ts>>...>) &&
            std::conjunction_v<detail::variant_move_assignable<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us&&>...>
    constexpr rvariant& operator=(rvariant<Us...>&& rhs)
        noexcept(std::conjunction_v<detail::variant_nothrow_move_assignable<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us&&>...>)
    {
        std::move(rhs).raw_visit([this]<std::size_t j, class Uj>(std::in_place_index_t<j>, [[maybe_unused]] Uj&& uj)
            noexcept(std::conjunction_v<detail::variant_nothrow_move_assignable<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us&&>...>)
        {
            if constexpr (j == std::variant_npos) {
                this->visit_reset();

            } else {
                using maybe_wrapped = detail::select_maybe_wrapped<unwrap_recursive_t<Uj>, Ts...>;
                using VT = typename maybe_wrapped::type;
                static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<Uj>>);

                this->raw_visit([this, &uj]<std::size_t i, class Ti>(std::in_place_index_t<i>, [[maybe_unused]] Ti& ti)
                    noexcept(std::conjunction_v<detail::variant_nothrow_move_assignable<detail::select_maybe_wrapped_t<unwrap_recursive_t<Us>, Ts...>, Us&&>...>)
                {
                    static_assert(std::is_rvalue_reference_v<Uj&&>);
                    constexpr std::size_t VTi = maybe_wrapped::index;
                    if constexpr (i == std::variant_npos) { // this is valueless, rhs holds value
                        base_type::template construct_on_valueless<VTi>(detail::forward_maybe_wrapped<VT>(std::move(uj)));  // NOLINT(bugprone-move-forwarding-reference)

                    } else if constexpr (std::is_same_v<unwrap_recursive_t<Ti>, unwrap_recursive_t<Uj>>) {
                        ti = detail::forward_maybe_wrapped<Ti>(std::move(uj)); // NOLINT(bugprone-move-forwarding-reference)

                    } else {
                        base_type::template reset_construct<i, VTi>(detail::forward_maybe_wrapped<VT>(std::move(uj)));  // NOLINT(bugprone-move-forwarding-reference)
                    }
                });
            }
        });
        return *this;
    }

    // ------------------------------------------------

    // in_place_type<T>, args...
    template<class T, class... Args>
        requires
            detail::non_wrapped_exactly_once_v<T, unwrapped_types> &&
            std::is_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, Args...>
    constexpr explicit rvariant(std::in_place_type_t<T>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, Args...>)
        : base_type(std::in_place_index<detail::select_maybe_wrapped_index<T, Ts...>>, std::forward<Args>(args)...)
    {}

    // in_place_type<T>, il, args...
    template<class T, class U, class... Args>
        requires
            detail::non_wrapped_exactly_once_v<T, unwrapped_types> &&
            std::is_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, std::initializer_list<U>&, Args...>
    constexpr explicit rvariant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, std::initializer_list<U>&, Args...>)
        : base_type(std::in_place_index<detail::select_maybe_wrapped_index<T, Ts...>>, il, std::forward<Args>(args)...)
    {}

    // in_place_index<I>, args...
    template<std::size_t I, class... Args>
        requires
            (I < sizeof...(Ts)) &&
            std::is_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>
    constexpr explicit rvariant(std::in_place_index_t<I>, Args&&... args) // NOLINT
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>)
        : base_type(std::in_place_index<I>, std::forward<Args>(args)...)
    {}

    // in_place_index<I>, il, args...
    template<std::size_t I, class U, class... Args>
        requires
            (I < sizeof...(Ts)) &&
            std::is_constructible_v<core::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
    constexpr explicit rvariant(std::in_place_index_t<I>, std::initializer_list<U> il, Args&&... args) // NOLINT
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>)
        : base_type(std::in_place_index<I>, il, std::forward<Args>(args)...)
    {}

    // -------------------------------------------

    template<class T, class... Args>
        requires
            detail::non_wrapped_exactly_once_v<T, unwrapped_types> &&
            std::is_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, Args...>
    constexpr T& emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, Args...>) YK_LIFETIMEBOUND
    {
        return this->template emplace<detail::select_maybe_wrapped_index<T, Ts...>>(std::forward<Args>(args)...);
    }

    template<class T, class U, class... Args>
        requires
            detail::non_wrapped_exactly_once_v<T, unwrapped_types> &&
            std::is_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, std::initializer_list<U>&, Args...>
    constexpr T& emplace(std::initializer_list<U> il, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, std::initializer_list<U>&, Args...>) YK_LIFETIMEBOUND
    {
        return this->template emplace<detail::select_maybe_wrapped_index<T, Ts...>>(il, std::forward<Args>(args)...);
    }

private:
    template<std::size_t I, class... Args>
        requires std::is_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>
    constexpr variant_alternative_t<I, rvariant>&
    emplace_impl(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>) YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        using T = core::pack_indexing_t<I, Ts...>;
        if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
            base_type::template reset_construct_never_valueless<I>(std::forward<Args>(args)...);

        } else {
        YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
            this->raw_visit([&, this]<std::size_t old_i, class T_old_i>(std::in_place_index_t<old_i>, T_old_i& t_old_i)
                noexcept(std::is_nothrow_constructible_v<T, Args...>)
            {
                static_assert(!std::is_reference_v<T_old_i>);
                static_assert(!std::is_const_v<T_old_i>);

                if constexpr (old_i == std::variant_npos) {
                    (void)t_old_i;
                    this->template construct_on_valueless<I>(std::forward<Args>(args)...);

                } else if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
                    t_old_i.~T_old_i();
                    static_assert(noexcept(std::construct_at(&this->storage(), std::in_place_index<old_i>, std::forward<Args>(args)...)));
                    std::construct_at(&this->storage(), std::in_place_index<old_i>, std::forward<Args>(args)...);

                } else if constexpr (std::is_same_v<T_old_i, T>) { // NOT type-changing
                    if constexpr (
                        (sizeof(T) <= detail::never_valueless_trivial_size_limit && std::is_trivially_move_assignable_v<T>) ||
                        core::is_ttp_specialization_of_v<T, recursive_wrapper>
                    ) {
                        T tmp(std::forward<Args>(args)...); // may throw
                        static_assert(noexcept(t_old_i = std::move(tmp)));
                        t_old_i = std::move(tmp);
                    } else if constexpr (
                        sizeof(T) <= detail::never_valueless_trivial_size_limit && std::is_trivially_copy_assignable_v<T>
                    ) { // strange type...
                        T const tmp(std::forward<Args>(args)...); // may throw
                        static_assert(noexcept(t_old_i = tmp));
                        t_old_i = tmp;
                    } else {
                        static_assert(!base_type::never_valueless);
                        t_old_i.~T_old_i();
                        this->index_ = detail::variant_npos<sizeof...(Ts)>;
                        static_assert(!noexcept(std::construct_at(&this->storage(), std::in_place_index<old_i>, std::forward<Args>(args)...)));
                        std::construct_at(&this->storage(), std::in_place_index<old_i>, std::forward<Args>(args)...); // may throw
                        this->index_ = old_i;
                    }

                } else { // type-changing
                    if constexpr (
                        (sizeof(T) <= detail::never_valueless_trivial_size_limit && std::is_trivially_move_constructible_v<T>) ||
                        core::is_ttp_specialization_of_v<T, recursive_wrapper>
                    ) {
                        T tmp(std::forward<Args>(args)...); // may throw
                        t_old_i.~T_old_i();
                        static_assert(noexcept(std::construct_at(&this->storage(), std::in_place_index<I>, std::move(tmp))));
                        std::construct_at(&this->storage(), std::in_place_index<I>, std::move(tmp)); // never throws
                        this->index_ = I;
                    } else if constexpr (
                        sizeof(T) <= detail::never_valueless_trivial_size_limit && std::is_trivially_copy_constructible_v<T>
                    ) { // strange type...
                        T const tmp(std::forward<Args>(args)...); // may throw
                        t_old_i.~T_old_i();
                        static_assert(noexcept(std::construct_at(&this->storage(), std::in_place_index<I>, tmp)));
                        std::construct_at(&this->storage(), std::in_place_index<I>, tmp); // never throws
                        this->index_ = I;
                    } else {
                        static_assert(!base_type::never_valueless);
                        t_old_i.~T_old_i();
                        this->index_ = detail::variant_npos<sizeof...(Ts)>;
                        static_assert(!noexcept(std::construct_at(&this->storage(), std::in_place_index<I>, std::forward<Args>(args)...)));
                        std::construct_at(&this->storage(), std::in_place_index<I>, std::forward<Args>(args)...); // may throw
                        this->index_ = I;
                    }
                }
            });
        YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END
        }
        return detail::unwrap_recursive(detail::raw_get<I>(storage()));
    }

public:
    template<std::size_t I, class... Args>
        requires std::is_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>
    constexpr variant_alternative_t<I, rvariant>&
    emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>) YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        return this->template emplace_impl<I>(std::forward<Args>(args)...);
    }

    template<std::size_t I, class U, class... Args>
        requires std::is_constructible_v<core::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
    constexpr variant_alternative_t<I, rvariant>&
    emplace(std::initializer_list<U> il, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>) YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        return this->template emplace_impl<I>(il, std::forward<Args>(args)...);
    }


    constexpr void swap(rvariant& rhs)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>..., std::is_nothrow_swappable<Ts>...>)
    {
        static_assert(std::conjunction_v<std::is_move_constructible<Ts>...>);
        static_assert(std::conjunction_v<std::is_swappable<Ts>...>);
        [[maybe_unused]] static constexpr bool all_nothrow_swappable = std::conjunction_v<std::is_nothrow_move_constructible<Ts>..., std::is_nothrow_swappable<Ts>...>;

        if constexpr (std::conjunction_v<core::is_trivially_swappable<Ts>...>) {
            static_assert(core::is_trivially_swappable_v<decltype(storage())>);
            std::swap(storage(), rhs.storage()); // no ADL
            std::swap(index_, rhs.index_);

        } else if constexpr (sizeof...(Ts) * sizeof...(Ts) < detail::visit_instantiation_limit) {
        YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_BEGIN
            this->raw_visit([this, &rhs]<std::size_t i, class ThisAlt>(std::in_place_index_t<i>, [[maybe_unused]] ThisAlt& this_alt)
                noexcept(all_nothrow_swappable)
            {
                rhs.raw_visit([this, &rhs, &this_alt]<std::size_t j, class RhsAlt>(std::in_place_index_t<j>, [[maybe_unused]] RhsAlt& rhs_alt)
                    noexcept(all_nothrow_swappable)
                {
                    if constexpr (i == j) {
                        if constexpr (i != std::variant_npos) {
                            using std::swap;
                            swap(this_alt, rhs_alt);
                        }

                    } else if constexpr (i == std::variant_npos) {
                        this->template construct_on_valueless<j>(std::move(rhs_alt));
                        rhs.template reset<j>();
                        (void)this_alt;

                    } else if constexpr (j == std::variant_npos) {
                        rhs.template construct_on_valueless<i>(std::move(this_alt));
                        this->template reset<i>();

                    } else {
                        auto tmp = std::move(this_alt);
                        this->template reset<i>();
                        this->template construct_on_valueless<j>(std::move(rhs_alt));
                        rhs.template reset<j>();
                        rhs.template construct_on_valueless<i>(std::move(tmp));
                    }
                });
            });
        YK_RVARIANT_ALWAYS_THROWING_UNREACHABLE_END
        } else {
            if (index_ == rhs.index_) {
                rhs.raw_visit([this]<std::size_t i, class RhsAlt>(std::in_place_index_t<i>, [[maybe_unused]] RhsAlt& rhs_alt)
                    noexcept(std::conjunction_v<std::is_nothrow_swappable<Ts>...>)
                {
                    if constexpr (i != std::variant_npos) {
                        using std::swap;
                        swap(detail::raw_get<i>(this->storage()), rhs_alt);
                    }
                });
            } else {
                auto tmp = std::move(*this);
                this->reset_steal_from(std::move(rhs));
                rhs.reset_steal_from(std::move(tmp));
            }
        }
    }

    friend constexpr void swap(rvariant& v, rvariant& w)
        noexcept(noexcept(v.swap(w)))
        requires (std::conjunction_v<std::conjunction<std::is_move_constructible<Ts>, std::is_swappable<Ts>>...>)
    {
        v.swap(w);
    }

    template<class... Us>
        requires std::is_same_v<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant subset() const& noexcept(std::is_nothrow_copy_constructible_v<rvariant>)
    {
        return *this;
    }

    template<class... Us>
        requires std::is_same_v<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant subset() && noexcept(std::is_nothrow_move_constructible_v<rvariant>)
    {
        return std::move(*this);
    }

    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant<Us...> subset() const&
        noexcept(
            rvariant_set::equivalent_to<rvariant<Us...>, rvariant> &&
            std::is_nothrow_constructible_v<rvariant<Us...>, rvariant const&> // equivalent to flexible copy constructor
        )
    {
        if constexpr (rvariant_set::equivalent_to<rvariant<Us...>, rvariant>) {
            return this->raw_visit([]<std::size_t i, class Ti>(std::in_place_index_t<i>, [[maybe_unused]] Ti const& ti) static
                noexcept(std::is_nothrow_constructible_v<rvariant<Us...>, rvariant const&>) -> rvariant<Us...>
            {
                if constexpr (i == std::variant_npos) {
                    return rvariant<Us...>(detail::valueless);
                } else {
                    constexpr std::size_t j = detail::subset_reindex<rvariant, rvariant<Us...>>(i);
                    static_assert(j != core::find_npos);
                    return rvariant<Us...>(std::in_place_index<j>, ti);
                }
            });
        } else {
            return this->raw_visit([]<std::size_t i, class Ti>(std::in_place_index_t<i>, [[maybe_unused]] Ti const& ti) static
                /* not noexcept */ -> rvariant<Us...>
            {
                if constexpr (i == std::variant_npos) {
                    return rvariant<Us...>(detail::valueless);
                } else {
                    constexpr std::size_t j = detail::subset_reindex<rvariant, rvariant<Us...>>(i);
                    if constexpr (j == core::find_npos) {
                        throw std::bad_variant_access{};
                    } else {
                        return rvariant<Us...>(std::in_place_index<j>, ti);
                    }
                }
            });
        }
    }

    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant<Us...> subset() &&
        noexcept(
            rvariant_set::equivalent_to<rvariant<Us...>, rvariant> &&
            std::is_nothrow_constructible_v<rvariant<Us...>, rvariant&&> // equivalent to flexible move constructor
        )
    {
        if constexpr (rvariant_set::equivalent_to<rvariant<Us...>, rvariant>) {
            return std::move(*this).raw_visit([]<std::size_t i, class Ti>(std::in_place_index_t<i>, [[maybe_unused]] Ti&& ti) static
                noexcept(std::is_nothrow_constructible_v<rvariant<Us...>, rvariant&&>) -> rvariant<Us...>
            {
                if constexpr (i == std::variant_npos) {
                    return rvariant<Us...>(detail::valueless);
                } else {
                    constexpr std::size_t j = detail::subset_reindex<rvariant, rvariant<Us...>>(i);
                    static_assert(j != core::find_npos);
                    static_assert(std::is_rvalue_reference_v<Ti&&>);
                    return rvariant<Us...>(std::in_place_index<j>, std::move(ti)); // NOLINT(bugprone-move-forwarding-reference)
                }
            });
        } else {
            return std::move(*this).raw_visit([]<std::size_t i, class Ti>(std::in_place_index_t<i>, [[maybe_unused]] Ti&& ti) static
                /* not noexcept */ -> rvariant<Us...>
            {
                if constexpr (i == std::variant_npos) {
                    return rvariant<Us...>(detail::valueless);
                } else {
                    constexpr std::size_t j = detail::subset_reindex<rvariant, rvariant<Us...>>(i);
                    if constexpr (j == core::find_npos) {
                        throw std::bad_variant_access{};
                    } else {
                        static_assert(std::is_rvalue_reference_v<Ti&&>);
                        return rvariant<Us...>(std::in_place_index<j>, std::move(ti)); // NOLINT(bugprone-move-forwarding-reference)
                    }
                }
            });
        }
    }

    // NOLINTBEGIN(cppcoreguidelines-missing-std-forward)

    // Member `.visit(...)`
    // <https://eel.is/c++draft/variant.visit#lib:visit,variant_>
    template<int = 0, class Self, class Visitor>
    constexpr decltype(auto) visit(this Self&& self, Visitor&& vis)
    {
        using copy_const_V = std::conditional_t<
            std::is_const_v<std::remove_reference_t<Self>>,
            rvariant const,
            rvariant
        >;
        using V = std::conditional_t<
            std::is_rvalue_reference_v<Self&&>,
            copy_const_V&&,
            copy_const_V&
        >;
        // ReSharper disable once CppCStyleCast
        return yk::visit(std::forward<Visitor>(vis), (V)self);
    }

    // Member `.visit<R>(...)`
    // <https://eel.is/c++draft/variant.visit#lib:visit,variant__>
    template<class R, class Self, class Visitor>
    constexpr R visit(this Self&& self, Visitor&& vis)
    {
        using copy_const_V = std::conditional_t<
            std::is_const_v<std::remove_reference_t<Self>>,
            rvariant const,
            rvariant
        >;
        using V = std::conditional_t<
            std::is_rvalue_reference_v<Self&&>,
            copy_const_V&&,
            copy_const_V&
        >;
        // ReSharper disable once CppCStyleCast
        return yk::visit<R>(std::forward<Visitor>(vis), (V)self);
    }

    // NOLINTEND(cppcoreguidelines-missing-std-forward)

    template<class Compare, class... Ts_>
    friend constexpr bool detail::compare_relops(rvariant<Ts_...> const&, rvariant<Ts_...> const&)
        noexcept(std::conjunction_v<std::is_nothrow_invocable<Compare, Ts_ const&, Ts_ const&>...>);

    template<class... Ts_>
        requires (std::three_way_comparable<Ts_> && ...)
    friend constexpr std::common_comparison_category_t<std::compare_three_way_result_t<Ts_>...> operator<=>(rvariant<Ts_...> const&, rvariant<Ts_...> const&)
        noexcept(std::conjunction_v<std::is_nothrow_invocable<std::compare_three_way, Ts_ const&, Ts_ const&>...>);

    template<class... Us>
    friend class rvariant;

    template<class Variant, class T>
    friend struct detail::exactly_once_index;

    template<class Variant>
    friend struct detail::forward_storage_t_impl;

    template<class Variant>
    friend struct detail::forward_storage_t_impl;

    template<class Variant>
    friend constexpr detail::forward_storage_t<Variant>&& detail::forward_storage(std::remove_reference_t<Variant>& v YK_LIFETIMEBOUND) noexcept;

    template<class Variant>
    friend constexpr detail::forward_storage_t<Variant>&& detail::forward_storage(std::remove_reference_t<Variant>&& v YK_LIFETIMEBOUND) noexcept;

    template<class Variant, class Visitor>
    friend constexpr detail::raw_visit_return_type<Visitor, detail::forward_storage_t<Variant>>
    detail::raw_visit(Variant&&, Visitor&&) noexcept(detail::raw_visit_noexcept<Visitor, detail::forward_storage_t<Variant>>);  // NOLINT(clang-diagnostic-microsoft-exception-spec)

    template<class R, class V, std::size_t... n>
    friend struct detail::visit_impl;

private:
    // hack: reduce compile error by half on unrelated overloads
    template<std::same_as<detail::valueless_t> Valueless>
    constexpr explicit rvariant(Valueless const&) noexcept
        : base_type(detail::valueless)
    {}

    template<class From, class To>
    friend consteval std::size_t detail::subset_reindex(std::size_t index) noexcept;
};

// -------------------------------------------------

template<class T, class... Ts>
    requires core::is_ttp_specialization_of_v<T, recursive_wrapper>
[[nodiscard]] constexpr bool holds_alternative(rvariant<Ts...> const& v) noexcept = delete;

template<class T, class... Ts>
[[nodiscard]] constexpr bool holds_alternative(rvariant<Ts...> const& v) noexcept
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    return v.index() == I;
}

// -------------------------------------------------

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr variant_alternative_t<I, rvariant<Ts...>>&
get(rvariant<Ts...>& v YK_LIFETIMEBOUND)
{
    if (I != v.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...>&>(v)));
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr variant_alternative_t<I, rvariant<Ts...>>&&
get(rvariant<Ts...>&& v YK_LIFETIMEBOUND)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    if (I != v.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...>&&>(v)));
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr variant_alternative_t<I, rvariant<Ts...>> const&
get(rvariant<Ts...> const& v YK_LIFETIMEBOUND)
{
    if (I != v.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...> const&>(v)));
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr variant_alternative_t<I, rvariant<Ts...>> const&&
get(rvariant<Ts...> const&& v YK_LIFETIMEBOUND)
{
    if (I != v.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...> const&&>(v)));
}

template<class T, class... Ts>
[[nodiscard]] constexpr T&
get(rvariant<Ts...>& v YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (v.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...>&>(v)));
}

template<class T, class... Ts>
[[nodiscard]] constexpr T&&
get(rvariant<Ts...>&& v YK_LIFETIMEBOUND)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (v.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...>&&>(v)));
}

template<class T, class... Ts>
[[nodiscard]] constexpr T const&
get(rvariant<Ts...> const& v YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (v.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...> const&>(v)));
}

template<class T, class... Ts>
[[nodiscard]] constexpr T const&&
get(rvariant<Ts...> const&& v YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (v.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...> const&&>(v)));
}

template<class T, class... Ts>
    requires core::is_ttp_specialization_of_v<T, recursive_wrapper>
constexpr T& get(rvariant<Ts...>&) = delete;

template<class T, class... Ts>
    requires core::is_ttp_specialization_of_v<T, recursive_wrapper>
constexpr T&& get(rvariant<Ts...>&&) = delete;

template<class T, class... Ts>
    requires core::is_ttp_specialization_of_v<T, recursive_wrapper>
constexpr T const& get(rvariant<Ts...> const&) = delete;

template<class T, class... Ts>
    requires core::is_ttp_specialization_of_v<T, recursive_wrapper>
constexpr T const&& get(rvariant<Ts...> const&&) = delete;

// ---------------------------------------------

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts...>>>
get_if(rvariant<Ts...>* v) noexcept
{
    if (v == nullptr || v->index() != I) return nullptr;
    return std::addressof(detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...>&>(*v))));
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts...>> const>
get_if(rvariant<Ts...> const* v) noexcept
{
    if (v == nullptr || v->index() != I) return nullptr;
    return std::addressof(detail::unwrap_recursive(detail::raw_get<I>(detail::forward_storage<rvariant<Ts...> const&>(*v))));
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts...>>>
get(rvariant<Ts...>* v) noexcept
{
    return get_if<I>(v);
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts...>> const>
get(rvariant<Ts...> const* v) noexcept
{
    return get_if<I>(v);
}

template<class T, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<T>
get_if(rvariant<Ts...>* v) noexcept
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    return get_if<I>(v);
}

template<class T, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<T const>
get_if(rvariant<Ts...> const* v) noexcept
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    return get_if<I>(v);
}

template<class T, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<T>
get(rvariant<Ts...>* v) noexcept
{
    return get_if<T>(v);
}

template<class T, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<T const>
get(rvariant<Ts...> const* v) noexcept
{
    return get_if<T>(v);
}

template<class... Ts, class Compare = std::equal_to<>>
    requires std::conjunction_v<std::is_invocable_r<bool, Compare, Ts, Ts>...>
[[nodiscard]] constexpr bool operator==(rvariant<Ts...> const& v, rvariant<Ts...> const& w)
    noexcept(noexcept(detail::compare_relops<Compare>(v, w)))
{
    if (v.index() != w.index()) return false;
    return detail::compare_relops<Compare>(v, w);
}

template<class... Ts, class Compare = std::not_equal_to<>>
    requires std::conjunction_v<std::is_invocable_r<bool, Compare, Ts, Ts>...>
[[nodiscard]] constexpr bool operator!=(rvariant<Ts...> const& v, rvariant<Ts...> const& w)
    noexcept(noexcept(detail::compare_relops<Compare>(v, w)))
{
    if (v.index() != w.index()) return true;
    return detail::compare_relops<Compare>(v, w);
}

template<class... Ts, class Compare = std::less<>>
    requires std::conjunction_v<std::is_invocable_r<bool, Compare, Ts, Ts>...>
[[nodiscard]] constexpr bool operator<(rvariant<Ts...> const& v, rvariant<Ts...> const& w)
    noexcept(noexcept(detail::compare_relops<Compare>(v, w)))
{
    auto const vi = detail::valueless_bias<rvariant<Ts...>::never_valueless>(v.index());
    auto const wi = detail::valueless_bias<rvariant<Ts...>::never_valueless>(w.index());
    if (vi < wi) return true;
    if (vi > wi) return false;
    return detail::compare_relops<Compare>(v, w);
}

template<class... Ts, class Compare = std::greater<>>
    requires std::conjunction_v<std::is_invocable_r<bool, Compare, Ts, Ts>...>
[[nodiscard]] constexpr bool operator>(rvariant<Ts...> const& v, rvariant<Ts...> const& w)
    noexcept(noexcept(detail::compare_relops<Compare>(v, w)))
{
    auto const vi = detail::valueless_bias<rvariant<Ts...>::never_valueless>(v.index());
    auto const wi = detail::valueless_bias<rvariant<Ts...>::never_valueless>(w.index());
    if (vi > wi) return true;
    if (vi < wi) return false;
    return detail::compare_relops<Compare>(v, w);
}

template<class... Ts, class Compare = std::less_equal<>>
    requires std::conjunction_v<std::is_invocable_r<bool, Compare, Ts, Ts>...>
[[nodiscard]] constexpr bool operator<=(rvariant<Ts...> const& v, rvariant<Ts...> const& w)
    noexcept(noexcept(detail::compare_relops<Compare>(v, w)))
{
    auto const vi = detail::valueless_bias<rvariant<Ts...>::never_valueless>(v.index());
    auto const wi = detail::valueless_bias<rvariant<Ts...>::never_valueless>(w.index());
    if (vi < wi) return true;
    if (vi > wi) return false;
    return detail::compare_relops<Compare>(v, w);
}

template<class... Ts, class Compare = std::greater_equal<>>
    requires std::conjunction_v<std::is_invocable_r<bool, Compare, Ts, Ts>...>
[[nodiscard]] constexpr bool operator>=(rvariant<Ts...> const& v, rvariant<Ts...> const& w)
    noexcept(noexcept(detail::compare_relops<Compare>(v, w)))
{
    auto const vi = detail::valueless_bias<rvariant<Ts...>::never_valueless>(v.index());
    auto const wi = detail::valueless_bias<rvariant<Ts...>::never_valueless>(w.index());
    if (vi > wi) return true;
    if (vi < wi) return false;
    return detail::compare_relops<Compare>(v, w);
}

template<class... Ts>
    requires (std::three_way_comparable<Ts> && ...)
[[nodiscard]] constexpr std::common_comparison_category_t<std::compare_three_way_result_t<Ts>...>
operator<=>(rvariant<Ts...> const& v, rvariant<Ts...> const& w)
    noexcept(std::conjunction_v<std::is_nothrow_invocable<std::compare_three_way, Ts const&, Ts const&>...>)
{
    if (v.valueless_by_exception() || w.valueless_by_exception()) [[unlikely]] {
        return w.valueless_by_exception() <=> v.valueless_by_exception();
    }
    if (auto c = v.index() <=> w.index(); c != 0) return c;
    return v.raw_visit([&w]<std::size_t i, class T>(std::in_place_index_t<i>, T const& alt)
        noexcept(std::conjunction_v<std::is_nothrow_invocable<std::compare_three_way, Ts const&, Ts const&>...>)
        -> std::common_comparison_category_t<std::compare_three_way_result_t<Ts>...>
    {
        if constexpr (i != std::variant_npos) {
            return alt <=> detail::raw_get<i>(w.storage());
        } else {
            return std::strong_ordering::equivalent;
        }
    });
}

}  // yk


namespace std {

// https://eel.is/c++draft/variant.hash
template<class... Ts>
    requires std::conjunction_v<::yk::core::is_hash_enabled<std::remove_const_t<Ts>>...>
struct hash<::yk::rvariant<Ts...>>  // NOLINT(cert-dcl58-cpp)
{
    [[nodiscard]] static /* constexpr */ std::size_t operator()(::yk::rvariant<Ts...> const& v)
        noexcept(std::conjunction_v<::yk::core::is_nothrow_hashable<std::remove_const_t<Ts>>...>)
    {
        return ::yk::detail::raw_visit(v, []<std::size_t i, class T>(std::in_place_index_t<i>, T const& t) static
            noexcept(std::disjunction_v<
                std::bool_constant<i == std::variant_npos>,
                ::yk::core::is_nothrow_hashable<T>
            >)
        {
            if constexpr (i == std::variant_npos) {
                // Arbitrary value. Might be better to not use common values like
                // `0` or `-1`, because some hash implementations yield the re-interpreted
                // integral representation for fundamental types.
                (void)t;
                return 0xbaddeadbeefuz;

            } else {
                // Assume x64 for the description below. This assumption is solely for
                // demonstration, and the issue described below applies to any architecture.
                //
                // Let `Int` denote a strong typedef of `int` such that:
                //   -- The specialization `std::hash<Int>` is _enabled_ ([unord.hash]), and
                //   -- such specialization yields the same value as the underlying type.
                //
                // Statement 1.
                // For any standard library implementation,
                //   hash{}(variant<int, Int>{std::in_place_type<int>, 0}) ==
                //   hash{}(variant<int, Int>{std::in_place_type<Int>, 0})
                // yields false-positive `true` if `v.index()` is not hash-mixed.
                //
                // Statement 2.
                // Additionally, for any standard library implementation where
                //   hash{}(int(0)) == hash(unsigned(0)) // true in GCC/Clang/MSVC
                // is true, then:
                //   hash{}(variant<int, unsigned>{std::in_place_type<int>, 0}) ==
                //   hash{}(variant<int, unsigned>{std::in_place_type<unsigned>, 0})
                // yields false-positive `true` if `v.index()` is not hash-mixed.
                //
                // Statement 3.
                // Furthermore, for any standard library implementation where
                // the hash function does not consider the type's actual bit width, i.e.:
                //   hash{}(int(0)) == hash{}(long long(0)) // true in GCC/Clang, false in MSVC
                // the following expression:
                //   hash{}(variant<int, long long>{std::in_place_type<int>, 0}) ==
                //   hash{}(variant<int, long long>{std::in_place_type<long long>, 0})
                // yields false-positive `true` if `v.index()` is not hash-mixed.
                //
                // For the statements 1 and 2, one may embrace the status quo and just live
                // with hash collisions. However, for the statement 3, it is actually
                // HARMFUL because an end-user will face observable performance issues
                // just by switching their compiler from MSVC to GCC/Clang.
                //
                // Demo: https://godbolt.org/z/aKhs4vbco
                //
                // All above issues can be eliminated by simply returning
                //   `HC(v.index(), GET<v.index()>(v))`
                // where `HC` is an arbitrary hash mixer function.

                // We assume `hash_combine` is unnecessary here, since the collision
                // is very unlikely to occur as long as the `index_hash` is NOT
                // evaluated as the re-interpreted bit representation.
                constexpr std::size_t index_hash = ::yk::FNV_hash<>::hash(i);
                return index_hash + std::hash<T>{}(t);
            }
        });
    }
};

} // std


namespace yk {

template<class... Ts>
    requires std::conjunction_v<core::is_hash_enabled<std::remove_const_t<Ts>>...>
[[nodiscard]] std::size_t hash_value(rvariant<Ts...> const& v)
    noexcept(std::conjunction_v<core::is_nothrow_hashable<std::remove_const_t<Ts>>...>)
{
    return std::hash<rvariant<Ts...>>{}(v);
}

} // yk

#endif
