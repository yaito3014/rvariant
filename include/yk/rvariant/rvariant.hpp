#ifndef YK_RVARIANT_RVARIANT_HPP
#define YK_RVARIANT_RVARIANT_HPP

#include "rvariant.hpp"

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/detail/variant_storage.hpp>
#include <yk/rvariant/detail/recursive_traits.hpp>
#include <yk/rvariant/variant_helper.hpp>
#include <yk/rvariant/subset.hpp>

#include <yk/core/type_traits.hpp>
#include <yk/core/cond_trivial.hpp>

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

template<class T, class... Ts>
struct has_recursive_wrapper_duplicate
    : std::false_type {};

template<class T, class Allocator, class... Ts>
struct has_recursive_wrapper_duplicate<recursive_wrapper<T, Allocator>, Ts...>
    : core::is_in<T, Ts...> {};


template<class T, class List>
struct non_wrapped_exactly_once : core::exactly_once<T, List>
{
    static_assert(
        !core::is_ttp_specialization_of_v<T, recursive_wrapper>,
        "Constructing a `recursive_wrapper` alternative with its full type as the tag is "
        "prohibited to avoid confusion; just specify `T` instead."
    );
};

template<class T, class List>
constexpr bool non_wrapped_exactly_once_v = non_wrapped_exactly_once<T, List>::value;


template<class T, class Variant>
struct exactly_once_index
{
    static_assert(core::exactly_once_v<T, typename Variant::unwrapped_types>, "T or recursive_wrapper<T> must occur exactly once in Ts...");
    static constexpr std::size_t value = core::find_index_v<T, typename Variant::unwrapped_types>;
};

template<class T, class Variant>
inline constexpr std::size_t exactly_once_index_v = exactly_once_index<T, Variant>::value;


template<class... Ts>
struct rvariant_base
{
    static constexpr bool need_destructor_call = !std::conjunction_v<std::is_trivially_destructible<Ts>...>;

public:
    using storage_type = variant_storage_for<Ts...>;

    // internal constructor; this is not the same as the most derived class' default constructor (which uses T0)
    constexpr rvariant_base() noexcept
        : storage_{} // value-initialize
        , index_{variant_npos}
    {}

    // Copy constructor
    constexpr void _copy_construct(rvariant_base const& w)
        noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<Ts>...>)
    {
        w.raw_visit([this]<std::size_t j, class T>([[maybe_unused]] alternative<j, T> const& alt)
            noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<Ts>...>)
        {
            if constexpr (j != std::variant_npos) {
                construct_on_valueless<j>(alt.value);
            }
        });
    }

    // Move constructor
    constexpr void _move_construct(rvariant_base&& w)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
    {
        std::move(w).raw_visit([this]<std::size_t j, class T>([[maybe_unused]] alternative<j, T>&& alt)
            noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
        {
            if constexpr (j != std::variant_npos) {
                construct_on_valueless<j>(std::move(alt).value);
            }
        });
    }

    // Copy assignment
    constexpr void _copy_assign(rvariant_base const& rhs)
        noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<Ts>..., std::is_nothrow_copy_assignable<Ts>...>)
    {
        rhs.raw_visit([this]<std::size_t j, class T>([[maybe_unused]] alternative<j, T> const& rhs_alt)
            noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<Ts>..., std::is_nothrow_copy_assignable<Ts>...>)
        {
            if constexpr (j == std::variant_npos) {
                visit_reset();
            } else {
                if (index_ == j) {
                    // (2.3)
                    raw_get<j>(storage()).value = rhs_alt.value;
                } else {
                    // Related:
                    // (C++17) 2904. Make variant move-assignment more exception safe https://cplusplus.github.io/LWG/issue2904

                    // copy(noexcept) && move(throw)    => A
                    // copy(noexcept) && move(noexcept) => A
                    // copy(throw)    && move(throw)    => A
                    // copy(throw)    && move(noexcept) => B
                    if constexpr (std::is_nothrow_copy_constructible_v<T> || !std::is_nothrow_move_constructible_v<T>) {
                        // A, (2.4): emplace<j>(rhs_alt.value)
                        reset_construct<j>(rhs_alt.value);
                    } else {
                        // B, (2.5): operator=(rvariant(rhs))

                        // ... is semantically equivalent to above
                        // auto tmp = rhs_alt.value;
                        // emplace<j>(std::move(tmp));

                        // ... is equivalent to:
                        auto tmp = rhs_alt.value;
                        reset_construct<j>(std::move(tmp));
                    }
                }
            }
        });
    }

    // Move assignment
    constexpr void _move_assign(rvariant_base&& rhs)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>..., std::is_nothrow_move_assignable<Ts>...>)
    {
        std::move(rhs).raw_visit([this]<std::size_t j, class T>([[maybe_unused]] alternative<j, T>&& rhs_alt)
            noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>..., std::is_nothrow_move_assignable<Ts>...>)
        {
            if constexpr (j == std::variant_npos) {
                visit_reset();

            } else {
                if (index_ == j) {
                    // (8.3)
                    raw_get<j>(storage()).value = std::move(rhs_alt).value;
                    // (10.2): index() will be j
                } else {
                    // (8.4): emplace<j>(std::move(rhs_alt).value)
                    reset_construct<j>(std::move(rhs_alt).value);
                    // (10.1): in case of exception, hold no value
                }
            }
        });
    }

    // -------------------------------------------------------

    // Primary constructor called from derived class
    template<std::size_t I, class... Args>
        requires std::is_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>
    constexpr explicit rvariant_base(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>)
        : storage_(std::in_place_index<I>, std::forward<Args>(args)...)
        , index_{static_cast<variant_index_t>(I)}
    {}

    // Primary constructor called from derived class
    constexpr explicit rvariant_base(valueless_t) noexcept
        : storage_{} // valueless
        , index_{variant_npos}
    {}

    [[nodiscard]] constexpr bool valueless_by_exception() const noexcept { return index_ == std::variant_npos; }
    [[nodiscard]] constexpr std::size_t index() const noexcept { return static_cast<std::size_t>(index_); }

    // internal; must be called only from the destructor of variant itself
    constexpr void destroy_final() noexcept
    {
        if constexpr (need_destructor_call) {
            this->raw_visit([this]<std::size_t i, class T>([[maybe_unused]] alternative<i, T>& alt) noexcept {
                if constexpr (i != variant_npos) {
                    alt.value.~T();
                }
            });
        }
    }

    // internal
    constexpr void visit_reset() noexcept
    {
        if constexpr (need_destructor_call) {
            this->raw_visit([this]<std::size_t i, class T>([[maybe_unused]] alternative<i, T>& alt) noexcept {
                if constexpr (i != variant_npos) {
                    alt.value.~T();
                    index_ = variant_npos;
                }
            });
        } else {
            index_ = variant_npos;
        }
    }

    // internal
    template<std::size_t I>
    constexpr void reset() noexcept
    {
        assert(index_ == I);
        if constexpr (I != std::variant_npos) {
            using T = core::pack_indexing_t<I, Ts...>;
            raw_get<I>(storage_).value.~T();
            index_ = variant_npos;
        }
    }

    template<std::size_t I, class... Args>
    constexpr void construct_on_valueless(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>)
    {
        static_assert(I != variant_npos);
        assert(index_ == variant_npos);
        std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...);
        index_ = I;
    }

    template<std::size_t I, class... Args>
    constexpr void reset_construct(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>)
    {
        static_assert(I != variant_npos);
        visit_reset();
        std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...);
        index_ = I;
    }

    // used in swap operation
    constexpr void reset_steal_from(rvariant_base&& rhs)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
    {
        visit_reset();

        std::move(rhs).raw_visit([this]<std::size_t i, class RhsAlt>([[maybe_unused]] alternative<i, RhsAlt>&& rhs_alt)
            noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
        {
            if constexpr (i != variant_npos) {
                this->template construct_on_valueless<i>(std::move(rhs_alt).value);
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
        constexpr auto const& table = raw_visit_dispatch_table<Visitor, Storage>::value;
        auto&& f = table[self.index_ + 1];
        return std::invoke(f, std::forward<Visitor>(vis), std::forward<Self>(self).storage());
    }

    storage_type storage_{}; // value-initialize
    variant_index_t index_ = variant_npos;
};


template<class... Ts>
struct rvariant_non_trivial_destructor : rvariant_base<Ts...>
{
    constexpr ~rvariant_non_trivial_destructor() noexcept
    {
        rvariant_non_trivial_destructor::rvariant_base::destroy_final();
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

}  // detail


template<class... Ts>
class rvariant : private detail::rvariant_base_t<Ts...>
{
    static_assert(
        !std::disjunction_v<detail::has_recursive_wrapper_duplicate<Ts, Ts...>...>,
        "rvariant cannot have both T and recursive_wrapper of T."
    );
    static_assert(std::conjunction_v<std::is_destructible<Ts>...>);
    static_assert(sizeof...(Ts) > 0);

    using unwrapped_types = core::type_list<unwrap_recursive_t<Ts>...>;

    using base_type = detail::rvariant_base_t<Ts...>;
    friend struct detail::rvariant_base<Ts...>;

    using base_type::storage;
    using base_type::index_;
    using base_type::raw_visit;
    using base_type::visit_reset;
    using base_type::reset;

public:
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

    // Generic constructor (non-wrapped T)
    // <https://eel.is/c++draft/variant#lib:variant,constructor___>
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
            std::is_assignable_v<core::aggregate_initialize_resolution_t<T, Ts...>&, T> &&
            std::is_constructible_v<core::aggregate_initialize_resolution_t<T, Ts...>, T>
    constexpr rvariant& operator=(T&& t)
        noexcept(
            std::is_nothrow_assignable_v<core::aggregate_initialize_resolution_t<T, Ts...>&, T> &&
            std::is_nothrow_constructible_v<core::aggregate_initialize_resolution_t<T, Ts...>, T>
        )
    {
        using Tj = core::aggregate_initialize_resolution_t<T, Ts...>; // either plain type or wrapped with recursive_wrapper
        constexpr std::size_t j = core::aggregate_initialize_resolution_index<T, Ts...>;
        static_assert(j != std::variant_npos);

        this->raw_visit([this, &t]<std::size_t i, class Alt>([[maybe_unused]] detail::alternative<i, Alt>& this_alt)
            noexcept(
                std::is_nothrow_assignable_v<Tj&, T> &&
                std::is_nothrow_constructible_v<Tj, T>
            )
        {
            if constexpr (i == j) {
                // (13.1)
                this_alt.value = std::forward<T>(t);
                // (16.1): valueless_by_exception will be `false` even if exception is thrown
            } else {
                if constexpr (std::is_nothrow_constructible_v<Tj, T> || !std::is_nothrow_move_constructible_v<Tj>) {
                    // (13.2)
                    //emplace<j>(std::forward<T>(t));
                    reset_construct<j>(std::forward<T>(t));
                    // (16.2): permitted to be valueless

                } else {
                    // Related:
                    // (C++23) 3585. Variant converting assignment with immovable alternative https://cplusplus.github.io/LWG/issue3585

                    // (13.3)
                    //emplace<j>(Tj(std::forward<T>(t)));
                    Tj tmp(std::forward<T>(t));
                    reset_construct<j>(std::move(tmp));
                    // (16.2): permitted to be valueless
                }
            }
        });
        return *this;
    }

    // --------------------------------------

    // Flexible copy constructor
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            rvariant_set::conjunction_for_v<rvariant, rvariant<Us...> const&, std::is_constructible>
    constexpr /* not explicit */ rvariant(rvariant<Us...> const& w)
        noexcept(rvariant_set::conjunction_for_v<rvariant, rvariant<Us...> const&, std::is_nothrow_constructible>)
    {
        w.raw_visit([this]<std::size_t j, class WT>([[maybe_unused]] detail::alternative<j, WT> const& alt)
            noexcept(rvariant_set::conjunction_for_v<rvariant, rvariant<Us...> const&, std::is_nothrow_constructible>)
        {
            if constexpr (j != std::variant_npos) {
                constexpr std::size_t i = subset_reindex_for<rvariant<Us...>>(j);
                using VT = detail::select_maybe_wrapped_t<unwrap_recursive_t<WT>, Ts...>;
                static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<WT>>);
                construct_on_valueless<i>(detail::forward_maybe_wrapped<VT>(alt.value));
            }
        });
    }

    // Flexible move constructor
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            rvariant_set::conjunction_for_v<rvariant, rvariant<Us...>&&, std::is_constructible>
    constexpr /* not explicit */ rvariant(rvariant<Us...>&& w)
        noexcept(rvariant_set::conjunction_for_v<rvariant, rvariant<Us...>&&, std::is_nothrow_constructible>)
    {
        std::move(w).raw_visit([this]<std::size_t j, class WT>([[maybe_unused]] detail::alternative<j, WT>&& alt)
            noexcept(rvariant_set::conjunction_for_v<rvariant, rvariant<Us...>&&, std::is_nothrow_constructible>)
        {
            if constexpr (j != std::variant_npos) {
                constexpr std::size_t i = subset_reindex_for<rvariant<Us...>>(j);
                using VT = detail::select_maybe_wrapped_t<unwrap_recursive_t<WT>, Ts...>;
                static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<WT>>);
                construct_on_valueless<i>(detail::forward_maybe_wrapped<VT>(std::move(alt).value));
            }
        });
    }

    // --------------------------------------

    // Flexible copy assignment operator
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            rvariant_set::conjunction_for_v<rvariant, rvariant<Us...> const&, std::is_constructible> &&
            rvariant_set::conjunction_for_v<rvariant&, rvariant<Us...> const&, std::is_assignable>
    constexpr rvariant& operator=(rvariant<Us...> const& rhs)
        noexcept(
            rvariant_set::conjunction_for_v<rvariant, rvariant<Us...> const&, std::is_nothrow_constructible> &&
            rvariant_set::conjunction_for_v<rvariant&, rvariant<Us...> const&, std::is_nothrow_assignable>
        )
    {
        rhs.raw_visit([this, &rhs]<std::size_t j, class Uj>([[maybe_unused]] detail::alternative<j, Uj> const& rhs_alt)
            noexcept(
                rvariant_set::conjunction_for_v<rvariant, rvariant<Us...> const&, std::is_nothrow_constructible> &&
                rvariant_set::conjunction_for_v<rvariant&, rvariant<Us...> const&, std::is_nothrow_assignable>
            )
        {
            if constexpr (j == std::variant_npos) {
                this->visit_reset();

            } else {
                static constexpr std::size_t corresponding_i = subset_reindex_for<rvariant<Us...>>(j);
                using VT = detail::select_maybe_wrapped_t<unwrap_recursive_t<Uj>, Ts...>;
                static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<Uj>>);

                this->raw_visit([this, &rhs, &rhs_alt]<std::size_t i, class ThisAlt>([[maybe_unused]] detail::alternative<i, ThisAlt>& this_alt)
                    noexcept(
                        rvariant_set::conjunction_for_v<rvariant, rvariant<Us...> const&, std::is_nothrow_constructible> &&
                        rvariant_set::conjunction_for_v<rvariant&, rvariant<Us...> const&, std::is_nothrow_assignable>
                    )
                {
                    if constexpr (i != std::variant_npos) {
                        if constexpr (std::is_same_v<unwrap_recursive_t<Uj>, unwrap_recursive_t<ThisAlt>>) {
                            this_alt.value = detail::forward_maybe_wrapped<ThisAlt>(rhs_alt.value);
                        } else {
                            if constexpr (std::is_nothrow_copy_constructible_v<Uj> || !std::is_nothrow_move_constructible_v<Uj>) {
                                //emplace<corresponding_i>(detail::forward_maybe_wrapped<VT>(rhs_alt.value));
                                reset_construct<corresponding_i>(detail::forward_maybe_wrapped<VT>(rhs_alt.value));
                            } else {
                                //this->operator=(rvariant<Us...>(rhs));
                                auto tmp = rhs_alt.value;
                                reset_construct<corresponding_i>(detail::forward_maybe_wrapped<VT>(std::move(tmp)));
                            }
                        }
                    } else {
                        if constexpr (std::is_nothrow_copy_constructible_v<Uj> || !std::is_nothrow_move_constructible_v<Uj>) {
                            //emplace<corresponding_i>(detail::forward_maybe_wrapped<VT>(rhs_alt.value));
                            reset_construct<corresponding_i>(detail::forward_maybe_wrapped<VT>(rhs_alt.value));
                        } else {
                            //this->operator=(rvariant<Us...>(rhs));
                            auto tmp = rhs_alt.value;
                            reset_construct<corresponding_i>(detail::forward_maybe_wrapped<VT>(std::move(tmp)));
                        }
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
            rvariant_set::conjunction_for_v<rvariant, rvariant<Us...>&&, std::is_constructible> &&
            rvariant_set::conjunction_for_v<rvariant&, rvariant<Us...>&&, std::is_assignable>
    constexpr rvariant& operator=(rvariant<Us...>&& rhs)
        noexcept(
            rvariant_set::conjunction_for_v<rvariant, rvariant<Us...>&&, std::is_nothrow_constructible> &&
            rvariant_set::conjunction_for_v<rvariant&, rvariant<Us...>&&, std::is_nothrow_assignable>
        )
    {
        std::move(rhs).raw_visit([this, &rhs]<std::size_t j, class Uj>([[maybe_unused]] detail::alternative<j, Uj>&& rhs_alt)
            noexcept(
                rvariant_set::conjunction_for_v<rvariant, rvariant<Us...>&&, std::is_nothrow_constructible> &&
                rvariant_set::conjunction_for_v<rvariant&, rvariant<Us...>&&, std::is_nothrow_assignable>
            )
        {
            if constexpr (j == std::variant_npos) {
                this->visit_reset();

            } else { // rhs holds something
                static constexpr std::size_t corresponding_i = subset_reindex_for<rvariant<Us...>>(j);
                using VT = detail::select_maybe_wrapped_t<unwrap_recursive_t<Uj>, Ts...>;
                static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<Uj>>);

                this->raw_visit([this, &rhs, &rhs_alt]<std::size_t i, class ThisAlt>([[maybe_unused]] detail::alternative<i, ThisAlt>& this_alt)
                    noexcept(
                        rvariant_set::conjunction_for_v<rvariant, rvariant<Us...>&&, std::is_nothrow_constructible> &&
                        rvariant_set::conjunction_for_v<rvariant&, rvariant<Us...>&&, std::is_nothrow_assignable>
                    )
                {
                    if constexpr (i != std::variant_npos) { // rhs holds something, and *this holds something
                        if constexpr (std::is_same_v<unwrap_recursive_t<Uj>, unwrap_recursive_t<ThisAlt>>) {
                            this_alt.value = detail::forward_maybe_wrapped<ThisAlt>(std::move(rhs_alt).value);
                        } else {
                            //emplace<corresponding_i>(detail::forward_maybe_wrapped<VT>(std::move(rhs_alt).value));
                            reset_construct<corresponding_i>(detail::forward_maybe_wrapped<VT>(std::move(rhs_alt).value));
                        }
                    } else { // rhs holds something, and *this is valueless
                        //emplace<corresponding_i>(detail::forward_maybe_wrapped<VT>(std::move(rhs_alt).value));
                        reset_construct<corresponding_i>(detail::forward_maybe_wrapped<VT>(std::move(rhs_alt).value));
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
            std::is_constructible_v<T, Args...> &&
            detail::non_wrapped_exactly_once_v<T, unwrapped_types>
    constexpr T& emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>) YK_LIFETIMEBOUND
    {
        constexpr std::size_t I = core::find_index_v<T, unwrapped_types>;
        reset_construct<I>(std::forward<Args>(args)...);
        return detail::unwrap_recursive(detail::raw_get<I>(storage()).value);
    }

    template<class T, class U, class... Args>
        requires
            std::is_constructible_v<T, std::initializer_list<U>&, Args...> &&
            detail::non_wrapped_exactly_once_v<T, unwrapped_types>
    constexpr T& emplace(std::initializer_list<U> il, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>) YK_LIFETIMEBOUND
    {
        constexpr std::size_t I = core::find_index_v<T, unwrapped_types>;
        reset_construct<I>(il, std::forward<Args>(args)...);
        return detail::unwrap_recursive(detail::raw_get<I>(storage()).value);
    }

    template<std::size_t I, class... Args>
        requires std::is_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>
    constexpr variant_alternative_t<I, rvariant>&
    emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, Args...>) YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        reset_construct<I>(std::forward<Args>(args)...);
        return detail::unwrap_recursive(detail::raw_get<I>(storage()).value);
    }

    template<std::size_t I, class U, class... Args>
        requires std::is_constructible_v<core::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
    constexpr variant_alternative_t<I, rvariant>&
    emplace(std::initializer_list<U> il, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<core::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>) YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        reset_construct<I>(il, std::forward<Args>(args)...);
        return detail::unwrap_recursive(detail::raw_get<I>(storage()).value);
    }

    constexpr void swap(rvariant& rhs)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>..., std::is_nothrow_swappable<Ts>...>)
    {
        static_assert(std::conjunction_v<std::is_move_constructible<Ts>...>);
        static_assert(std::conjunction_v<std::is_swappable<Ts>...>);
        constexpr bool all_nothrow_swappable = std::conjunction_v<std::is_nothrow_move_constructible<Ts>..., std::is_nothrow_swappable<Ts>...>;
        constexpr std::size_t instantiation_limit = 1024; // TODO: apply same logic to other visits with >= O(n^2) branch

        if constexpr (std::conjunction_v<core::is_trivially_swappable<Ts>...>) {
            static_assert(core::is_trivially_swappable_v<decltype(storage())>);
            std::swap(storage(), rhs.storage()); // no ADL
            std::swap(index_, rhs.index_);

        } else if constexpr (sizeof...(Ts) * sizeof...(Ts) < instantiation_limit) {
            this->raw_visit([this, &rhs]<std::size_t i, class ThisAlt>([[maybe_unused]] detail::alternative<i, ThisAlt>& this_alt)
                noexcept(all_nothrow_swappable)
            {
                rhs.raw_visit([this, &rhs, &this_alt]<std::size_t j, class RhsAlt>([[maybe_unused]] detail::alternative<j, RhsAlt>& rhs_alt)
                    noexcept(all_nothrow_swappable)
                {
                    if constexpr (i == j) {
                        if constexpr (i != detail::variant_npos) {
                            using std::swap;
                            swap(this_alt.value, rhs_alt.value);
                        }

                    } else if constexpr (i == detail::variant_npos) {
                        this->template construct_on_valueless<j>(std::move(rhs_alt.value));
                        rhs.template reset<j>();

                    } else if constexpr (j == detail::variant_npos) {
                        rhs.template construct_on_valueless<i>(std::move(this_alt.value));
                        this->template reset<i>();

                    } else {
                        auto tmp = std::move(this_alt.value);
                        this->template reset<i>();
                        this->template construct_on_valueless<j>(std::move(rhs_alt.value));
                        rhs.template reset<j>();
                        rhs.template construct_on_valueless<i>(std::move(tmp));
                    }
                });
            });
        } else {
            if (index_ == rhs.index_) {
                rhs.raw_visit([this]<std::size_t i, class RhsAlt>([[maybe_unused]] detail::alternative<i, RhsAlt>& rhs_alt)
                    noexcept(std::conjunction_v<std::is_nothrow_swappable<Ts>...>)
                {
                    if constexpr (i != detail::variant_npos) {
                        using std::swap;
                        swap(detail::raw_get<i>(storage()).value, rhs_alt.value);
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
            return this->raw_visit([]<std::size_t i, class Alt>([[maybe_unused]] detail::alternative<i, Alt> const& alt)
                noexcept(std::is_nothrow_constructible_v<rvariant<Us...>, rvariant const&>) -> rvariant<Us...>
            {
                if constexpr (i == detail::variant_npos) {
                    return rvariant<Us...>(detail::valueless);
                } else {
                    constexpr std::size_t j = detail::subset_reindex<rvariant, rvariant<Us...>>(i);
                    static_assert(j != core::find_npos);
                    return rvariant<Us...>(std::in_place_index<j>, alt.value);
                }
            });
        } else {
            return this->raw_visit([]<std::size_t i, class Alt>([[maybe_unused]] detail::alternative<i, Alt> const& alt)
                /* not noexcept */ -> rvariant<Us...>
            {
                if constexpr (i == detail::variant_npos) {
                    return rvariant<Us...>(detail::valueless);
                } else {
                    constexpr std::size_t j = detail::subset_reindex<rvariant, rvariant<Us...>>(i);
                    if constexpr (j == core::find_npos) {
                        throw std::bad_variant_access{};
                    } else {
                        return rvariant<Us...>(std::in_place_index<j>, alt.value);
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
            return std::move(*this).raw_visit([]<std::size_t i, class Alt>([[maybe_unused]] detail::alternative<i, Alt>&& alt)
                noexcept(std::is_nothrow_constructible_v<rvariant<Us...>, rvariant&&>) -> rvariant<Us...>
            {
                if constexpr (i == detail::variant_npos) {
                    return rvariant<Us...>(detail::valueless);
                } else {
                    constexpr std::size_t j = detail::subset_reindex<rvariant, rvariant<Us...>>(i);
                    static_assert(j != core::find_npos);
                    return rvariant<Us...>(std::in_place_index<j>, std::move(alt).value);
                }
            });
        } else {
            return std::move(*this).raw_visit([]<std::size_t i, class Alt>([[maybe_unused]] detail::alternative<i, Alt>&& alt)
                /* not noexcept */ -> rvariant<Us...>
            {
                if constexpr (i == detail::variant_npos) {
                    return rvariant<Us...>(detail::valueless);
                } else {
                    constexpr std::size_t j = detail::subset_reindex<rvariant, rvariant<Us...>>(i);
                    if constexpr (j == core::find_npos) {
                        throw std::bad_variant_access{};
                    } else {
                        return rvariant<Us...>(std::in_place_index<j>, std::move(alt).value);
                    }
                }
            });
        }
    }


    template<std::size_t I, class... Ts_> friend constexpr auto&& get(rvariant<Ts_...>&);
    template<std::size_t I, class... Ts_> friend constexpr auto&& get(rvariant<Ts_...> const&);
    template<std::size_t I, class... Ts_> friend constexpr auto&& get(rvariant<Ts_...>&&);
    template<std::size_t I, class... Ts_> friend constexpr auto&& get(rvariant<Ts_...> const&&);
    template<class T, class... Ts_> friend constexpr T& get(rvariant<Ts_...>&);
    template<class T, class... Ts_> friend constexpr T const& get(rvariant<Ts_...> const&);
    template<class T, class... Ts_> friend constexpr T&& get(rvariant<Ts_...>&&);
    template<class T, class... Ts_> friend constexpr T const&& get(rvariant<Ts_...> const&&);

    template<std::size_t I, class... Ts_> friend constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts_...>>> get_if(rvariant<Ts_...>*) noexcept;
    template<std::size_t I, class... Ts_> friend constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts_...>> const> get_if(rvariant<Ts_...> const*) noexcept;
    template<std::size_t I, class... Ts_> friend constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts_...>>> get(rvariant<Ts_...>*) noexcept;
    template<std::size_t I, class... Ts_> friend constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts_...>> const> get(rvariant<Ts_...> const*) noexcept;

    template<class T, class... Ts_> friend constexpr std::add_pointer_t<T> get_if(rvariant<Ts_...>*) noexcept;
    template<class T, class... Ts_> friend constexpr std::add_pointer_t<T const> get_if(rvariant<Ts_...> const*) noexcept;
    template<class T, class... Ts_> friend constexpr std::add_pointer_t<T> get(rvariant<Ts_...>*) noexcept;
    template<class T, class... Ts_> friend constexpr std::add_pointer_t<T const> get(rvariant<Ts_...> const*) noexcept;

    template<class... Us>
    friend class rvariant;

    template<class Variant, class T>
    friend struct detail::exactly_once_index;

private:
    // hack: reduce compile error by half on unrelated overloads
    template<std::same_as<detail::valueless_t> Valueless>
    constexpr explicit rvariant(Valueless const&) noexcept
        : base_type(detail::valueless)
    {}

    template<class From, class To>
    friend constexpr std::size_t detail::subset_reindex(std::size_t index) noexcept;

    template<class W>
    [[nodiscard]] static constexpr std::size_t subset_reindex_for(std::size_t index) noexcept
    {
        return detail::subset_reindex<W, rvariant>(index);
    }

    template<std::size_t I, class... Args>
    constexpr void reset_construct(Args&&... args)
        noexcept(noexcept(base_type::template reset_construct<I>(std::forward<Args>(args)...)))
    {
        return base_type::template reset_construct<I>(std::forward<Args>(args)...);
    }

    template<std::size_t I, class... Args>
    constexpr void construct_on_valueless(Args&&... args)
        noexcept(noexcept(base_type::template construct_on_valueless<I>(std::forward<Args>(args)...)))
    {
        return base_type::template construct_on_valueless<I>(std::forward<Args>(args)...);
    }
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
[[nodiscard]] constexpr auto&&
get(rvariant<Ts...>& v YK_LIFETIMEBOUND)
{
    if (I != v.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(v.storage_).value);
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr auto&&
get(rvariant<Ts...>&& v YK_LIFETIMEBOUND)
{
    if (I != v.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(v).storage_).value);
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr auto&&
get(rvariant<Ts...> const& v YK_LIFETIMEBOUND)
{
    if (I != v.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(v.storage_).value);
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr auto&&
get(rvariant<Ts...> const&& v YK_LIFETIMEBOUND)
{
    if (I != v.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(v).storage_).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T&
get(rvariant<Ts...>& v YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (v.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(v.storage_).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T&&
get(rvariant<Ts...>&& v YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (v.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(v).storage_).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T const&
get(rvariant<Ts...> const& v YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (v.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(v.storage_).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T const&&
get(rvariant<Ts...> const&& v YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (v.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(v).storage_).value);
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
    return std::addressof(detail::unwrap_recursive(detail::raw_get<I>(v->storage()).value));
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts...>> const>
get_if(rvariant<Ts...> const* v) noexcept
{
    if (v == nullptr || v->index() != I) return nullptr;
    return std::addressof(detail::unwrap_recursive(detail::raw_get<I>(v->storage()).value));
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

}  // namespace yk

#endif  // YK_RVARIANT_HPP
