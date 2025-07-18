#ifndef YK_RVARIANT_RVARIANT_HPP
#define YK_RVARIANT_RVARIANT_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/detail/variant_storage.hpp>
#include <yk/rvariant/detail/recursive_traits.hpp>
#include <yk/rvariant/variant_helper.hpp>
#include <yk/rvariant/subset.hpp>

#include <yk/detail/lang_core.hpp>

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

// https://eel.is/c++draft/variant#assign-5
template<class T>
struct variant_copy_assignable : std::bool_constant<
    std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>
> {};

template<class T>
struct variant_trivially_copy_assignable : std::bool_constant<
    std::is_trivially_copy_constructible_v<T> && std::is_trivially_copy_assignable_v<T> && std::is_trivially_destructible_v<T>
> {};


template<class T>
struct variant_move_assignable : std::bool_constant<
    std::is_move_constructible_v<T> && std::is_move_assignable_v<T>
> {};

template<class T>
struct variant_trivially_move_assignable : std::bool_constant<
    std::is_trivially_move_constructible_v<T> && std::is_trivially_move_assignable_v<T> && std::is_trivially_destructible_v<T>
> {};

template<class T>
struct variant_nothrow_move_assignable : std::bool_constant<
    std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>
> {};


template<class T>
struct variant_nothrow_swappable : std::bool_constant<
    std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>
> {};


// Not used because it ruins some type traits.
// Exists for historical reference.
#if 0
template<class Alt, class T>
struct FUN_body
{
    // https://eel.is/c++draft/variant#ctor-14

    /* constexpr */ T&& invent_ref(); // { return static_cast<T&&>(*static_cast<T*>(nullptr)); }
    Alt x[1] = {std::forward<T>(invent_ref())};

    // Does not work on non-MSVC
    //Alt x[1] = {std::forward<T>(std::declval<T>())};
};
#endif

template<std::size_t I, class Alt, class T>
struct FUN_overload
{
    using dest_array = Alt[]; // this was the existing vendors' approach

    constexpr std::integral_constant<std::size_t, I>
    operator()(Alt)
        requires requires(T&& t) {
            { dest_array{std::forward<T>(t)} }; // this was the existing vendors' approach

            // This appears to work, but certainly breaks something on MSVC
            // e.g. std::is_constructible_v<rvariant<int>, double> shows wrong value on Intellisense only
            //{ FUN_body<Alt, T>{} };
        }
    {
        return {}; // silence MSVC warning
    }
};

// imaginary function
template<class T, class Variant, class Seq = std::make_index_sequence<variant_size_v<Variant>>>
struct FUN;

template<class T, class... Ts, std::size_t... Is>
struct FUN<T, rvariant<Ts...>, std::index_sequence<Is...>>
    : FUN_overload<Is, pack_indexing_t<Is, Ts...>, T>...
{
    using FUN_overload<Is, pack_indexing_t<Is, Ts...>, T>::operator()...;
};

template<class T, class Variant>
using accepted_index = decltype(FUN<T, Variant>{}(std::declval<T>()));

template<class T, class Variant>
inline constexpr std::size_t accepted_index_v = accepted_index<T, Variant>::value;

template<class T, class Variant>
struct accepted_type;

template<class T, class... Ts>
struct accepted_type<T, rvariant<Ts...>> {
    using type = pack_indexing_t<accepted_index_v<T, rvariant<Ts...>>, Ts...>;
};

template<class T, class Variant>
using accepted_type_t = typename accepted_type<T, Variant>::type;


template<class T, class... Ts>
struct has_recursive_wrapper_duplicate
    : std::false_type {};

template<class T, class Allocator, class... Ts>
struct has_recursive_wrapper_duplicate<recursive_wrapper<T, Allocator>, Ts...>
    : is_in<T, Ts...> {};


template<class T, class List>
struct non_wrapped_exactly_once : exactly_once<T, List>
{
    static_assert(
        !is_ttp_specialization_of_v<T, recursive_wrapper>,
        "Constructing a `recursive_wrapper` alternative with its full type as the tag is "
        "prohibited to avoid confusion; just specify `T` instead."
    );
};

template<class T, class List>
constexpr bool non_wrapped_exactly_once_v = non_wrapped_exactly_once<T, List>::value;


template<class T, class Variant>
struct exactly_once_index
{
    static_assert(exactly_once_v<T, typename Variant::unwrapped_types>, "T or recursive_wrapper<T> must occur exactly once in Ts...");
    static constexpr std::size_t value = find_index_v<T, typename Variant::unwrapped_types>;
};

template<class T, class Variant>
inline constexpr std::size_t exactly_once_index_v = exactly_once_index<T, Variant>::value;

}  // detail


template<class... Ts>
class rvariant
{
    static_assert(
        !std::disjunction_v<detail::has_recursive_wrapper_duplicate<Ts, Ts...>...>,
        "rvariant cannot have both T and recursive_wrapper of T."
    );
    static_assert(std::conjunction_v<std::is_destructible<Ts>...>);
    static_assert(sizeof...(Ts) > 0);

    using unwrapped_types = detail::type_list<unwrap_recursive_t<Ts>...>;

public:
    // BUG on clang-tidy; false-positive.
    // NOLINTBEGIN(modernize-use-equals-default)

    // Default constructor
    constexpr rvariant() noexcept(std::is_nothrow_default_constructible_v<detail::pack_indexing_t<0, Ts...>>)
        requires std::is_default_constructible_v<detail::pack_indexing_t<0, Ts...>>
        : storage_{} // value-initialize
        , index_(0)
    {} // Postconditions: valueless_by_exception() is false and index() is 0.

    // NOLINTEND(modernize-use-equals-default)
    // end BUG on clang-tidy


    // Copy constructor (trivial)
    constexpr rvariant(rvariant const&) = default;

    // Copy constructor
    constexpr rvariant(rvariant const& w)
        requires
            std::conjunction_v<std::is_copy_constructible<Ts>...> &&
            (!std::conjunction_v<std::is_trivially_copy_constructible<Ts>...>) // flipped for shorter compile error
        : storage_(detail::valueless)
        , index_(detail::variant_npos)
    {
        detail::raw_visit(
            w.index_, // j
            [this]<std::size_t j, class T>([[maybe_unused]] detail::alternative<j, T> const& alt) {
                if constexpr (j != std::variant_npos) {
                    std::construct_at(&storage_, std::in_place_index<j>, alt.value);
                    index_ = j;
                }
            },
            w
        );
    }

    // Move constructor (trivial)
    constexpr rvariant(rvariant&&) = default;

    // Move constructor
    constexpr rvariant(rvariant&& w) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
        requires
            std::conjunction_v<std::is_move_constructible<Ts>...> &&
            (!std::conjunction_v<std::is_trivially_move_constructible<Ts>...>) // flipped for shorter compile error
        : storage_(detail::valueless)
        , index_(detail::variant_npos)
    {
        detail::raw_visit(
            w.index_, // j
            [this]<std::size_t j, class T>([[maybe_unused]] detail::alternative<j, T>&& alt) {
                if constexpr (j != std::variant_npos) {
                    std::construct_at(&storage_, std::in_place_index<j>, std::move(alt).value);
                    index_ = j;
                }
            },
            std::move(w)
        );
    }

    // Generic constructor => see below: "Generic assignment operator"

    // -------------------------------------------------

    // Flexible copy constructor
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            std::conjunction_v<std::is_copy_constructible<Us>...>
    constexpr /* not explicit */ rvariant(rvariant<Us...> const& w)
        // TODO: check noexcept specifier is correctly determined even if it is absent here
        : storage_(detail::valueless)
        , index_(detail::variant_npos)
    {
        detail::raw_visit(
            w.index_, // j
            [this]<std::size_t j, class WT>([[maybe_unused]] detail::alternative<j, WT> const& alt) {
                if constexpr (j != std::variant_npos) {
                    constexpr std::size_t i = subset_reindex_for<rvariant<Us...>>(j);
                    using VT = detail::select_maybe_wrapped_t<unwrap_recursive_t<WT>, Ts...>;
                    static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<WT>>);
                    std::construct_at(&storage_, std::in_place_index<i>, detail::rewrap_maybe_recursive<VT>(alt.value));
                    index_ = i;
                }
            },
            w
        );
    }

    // Flexible move constructor
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            std::conjunction_v<std::is_move_constructible<Us>...>
    constexpr /* not explicit */ rvariant(rvariant<Us...>&& w)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Us>...>) // TODO: possibly wrong
        : storage_(detail::valueless)
        , index_(detail::variant_npos)
    {
        detail::raw_visit(
            w.index_, // j
            [this]<std::size_t j, class WT>([[maybe_unused]] detail::alternative<j, WT>&& alt) {
                if constexpr (j != std::variant_npos) {
                    constexpr std::size_t i = subset_reindex_for<rvariant<Us...>>(j);
                    using VT = detail::select_maybe_wrapped_t<unwrap_recursive_t<WT>, Ts...>;
                    static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<WT>>);
                    std::construct_at(&storage_, std::in_place_index<i>, detail::rewrap_maybe_recursive<VT>(std::move(alt).value));
                    index_ = i;
                }
            },
            std::move(w)
        );
    }

    template<class... Us>
        requires (!(
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            std::conjunction_v<std::is_copy_constructible<Us>...>
        ))
    constexpr explicit rvariant(rvariant<Us...> const&) = delete; // Us... must be a subset of Ts...

    template<class... Us>
        requires (!(
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            std::conjunction_v<std::is_move_constructible<Us>...>
        ))
    constexpr explicit rvariant(rvariant<Us...>&&) = delete; // Us... must be a subset of Ts...

    // ------------------------------------------------

    // in_place_type<T>, args...
    template<class T, class... Args>
        requires
            detail::non_wrapped_exactly_once_v<T, unwrapped_types> &&
            std::is_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, Args...>
    constexpr explicit rvariant(std::in_place_type_t<T>, Args&&... args)
        : rvariant(std::in_place_index<detail::select_maybe_wrapped_index<T, Ts...>>, std::forward<Args>(args)...)
    {}

    // in_place_type<T>, il, args...
    template<class T, class U, class... Args>
        requires
            detail::non_wrapped_exactly_once_v<T, unwrapped_types> &&
            std::is_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, std::initializer_list<U>&, Args...>
    constexpr explicit rvariant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
        : rvariant(std::in_place_index<detail::select_maybe_wrapped_index<T, Ts...>>, il, std::forward<Args>(args)...)
    {}

    // in_place_index<I>, args...
    template<std::size_t I, class... Args>
        requires
            (I < sizeof...(Ts)) &&
            std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, Args...>
    constexpr explicit rvariant(std::in_place_index_t<I>, Args&&... args) // NOLINT
        : storage_(std::in_place_index<I>, std::forward<Args>(args)...)
        , index_(I)
    {}

    // in_place_index<I>, il, args...
    template<std::size_t I, class U, class... Args>
        requires
            (I < sizeof...(Ts)) &&
            std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
    constexpr explicit rvariant(std::in_place_index_t<I>, std::initializer_list<U> il, Args&&... args) // NOLINT
        : storage_(std::in_place_index<I>, il, std::forward<Args>(args)...)
        , index_(I)
    {}

    constexpr ~rvariant() = default;

    constexpr ~rvariant()
        requires (!std::conjunction_v<std::is_trivially_destructible<Ts>...>)
    {
        if (!valueless_by_exception()) {
            dynamic_destroy();
        }
    }

    constexpr rvariant& operator=(rvariant const&)
        requires std::conjunction_v<detail::variant_trivially_copy_assignable<Ts>...>
    = default;

    // copy assignment
    // https://eel.is/c++draft/variant#lib:operator=,variant
    constexpr rvariant& operator=(rvariant const& rhs)
        requires
            (!std::conjunction_v<detail::variant_trivially_copy_assignable<Ts>...>) &&
            std::conjunction_v<detail::variant_copy_assignable<Ts>...>
    {
        detail::raw_visit(
            rhs.index_, // j
            [this, &rhs]<std::size_t j, class T>([[maybe_unused]] detail::alternative<j, T> const& rhs_alt) {
                if constexpr (j == std::variant_npos) {
                    if (index_ != std::variant_npos) {
                        dynamic_reset();
                    }
                } else {
                    if (index_ == j) {
                        // (2.3)
                        detail::raw_get<j>(*this).value = rhs_alt.value;
                    } else {
                        // Related:
                        // (C++17) 2904. Make variant move-assignment more exception safe https://cplusplus.github.io/LWG/issue2904

                        // copy(noexcept) && move(throw)    => A
                        // copy(noexcept) && move(noexcept) => A
                        // copy(throw)    && move(throw)    => A
                        // copy(throw)    && move(noexcept) => B
                        if constexpr (std::is_nothrow_copy_constructible_v<T> || !std::is_nothrow_move_constructible_v<T>) {
                            // A, (2.4)
                            emplace<j>(rhs_alt.value);
                        } else {
                            // B, (2.5)
                            this->operator=(rvariant(rhs));
                        }
                    }
                }
            },
            rhs
        );
        return *this;
    }

    // --------------------------------------------

    constexpr rvariant& operator=(rvariant&&)
        requires std::conjunction_v<detail::variant_trivially_move_assignable<Ts>...>
    = default;

    // move assignment
    // https://eel.is/c++draft/variant#lib:operator=,variant_
    constexpr rvariant& operator=(rvariant&& rhs)
        noexcept(std::conjunction_v<detail::variant_nothrow_move_assignable<Ts>...>)
        requires
            (!std::conjunction_v<detail::variant_trivially_move_assignable<Ts>...>) &&
            std::conjunction_v<detail::variant_move_assignable<Ts>...>
    {
        detail::raw_visit(
            rhs.index_, // j
            [this]<std::size_t j, class T>([[maybe_unused]] detail::alternative<j, T>&& rhs_alt) {
                if constexpr (j == std::variant_npos) {
                    if (index_ != std::variant_npos) {
                        dynamic_reset();
                    }
                } else {
                    if (index_ == j) {
                        // (8.3)
                        detail::raw_get<j>(*this).value = std::move(rhs_alt).value;
                        // (10.2): index() will be j
                    } else {
                        // (8.4)
                        emplace<j>(std::move(rhs_alt).value);
                        // (10.1): will hold no value
                    }
                }
            },
            std::move(rhs)
        );
        return *this;
    }

    // --------------------------------------

    // Generic constructor (non-wrapped T)
    // <https://eel.is/c++draft/variant#lib:variant,constructor___>
    template<class T>
        requires
            (sizeof...(Ts) > 0) &&
            (!std::is_same_v<std::remove_cvref_t<T>, rvariant>) &&
            //(!detail::is_ttp_specialization_of_v<std::remove_cvref_t<T>, recursive_wrapper>) && // TODO: implement
            (!detail::is_ttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_type_t>) &&
            (!detail::is_nttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_index_t>) &&
            requires(T&& t) { detail::FUN<T, rvariant>{}(std::forward<T>(t)); } && // flipped order
            std::is_constructible_v<detail::accepted_type_t<T, rvariant>, T>
    constexpr /* not explicit */ rvariant(T&& t)
        noexcept(std::is_nothrow_constructible_v<detail::accepted_type_t<T, rvariant>, T>)
        : storage_(std::in_place_index<detail::accepted_index_v<T, rvariant>>, std::forward<T>(t))
        , index_(detail::accepted_index_v<T, rvariant>)
    {}

    // Generic assignment operator
    // <https://eel.is/c++draft/variant.assign#lib:operator=,variant__>
    template<class T>
        requires
            (!std::is_same_v<std::remove_cvref_t<T>, rvariant>) &&
            //(!detail::is_ttp_specialization_of_v<std::remove_cvref_t<T>, recursive_wrapper>) && // TODO: implement
            requires(T&& t) { detail::FUN<T, rvariant>{}(std::forward<T>(t)); } && // flipped order
            std::is_assignable_v<detail::accepted_type_t<T, rvariant>&, T> &&
            std::is_constructible_v<detail::accepted_type_t<T, rvariant>, T>
    constexpr rvariant& operator=(T&& t)
        noexcept(
            std::is_nothrow_assignable_v<detail::accepted_type_t<T, rvariant>&, T> &&
            std::is_nothrow_constructible_v<detail::accepted_type_t<T, rvariant>, T>
        )
    {
        using Tj = detail::accepted_type_t<T, rvariant>; // either plain type or wrapped with recursive_wrapper
        constexpr std::size_t j = detail::accepted_index_v<T, rvariant>;
        static_assert(j != std::variant_npos);

        detail::raw_visit(
            index_,
            [this, &t]<std::size_t i, class Alt>([[maybe_unused]] detail::alternative<i, Alt>& this_alt) {
                if constexpr (i == j) {
                    // (13.1)
                    this_alt.value = std::forward<T>(t);
                    // (16.1): valueless_by_exception will be `false` even if exception is thrown
                } else {
                    if constexpr (std::is_nothrow_constructible_v<Tj, T> || !std::is_nothrow_move_constructible_v<Tj>) {
                        // (13.2)
                        emplace<j>(std::forward<T>(t));
                        // (16.2): permitted to be valueless
                    } else {
                        // Related:
                        // (C++23) 3585. Variant converting assignment with immovable alternative https://cplusplus.github.io/LWG/issue3585

                        // (13.3)
                        emplace<j>(Tj(std::forward<T>(t)));
                        // (16.2): permitted to be valueless
                    }
                }
            },
            *this
        );
        return *this;
    }

    // --------------------------------------

    // Flexible copy assignment operator
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            std::conjunction_v<detail::variant_copy_assignable<Us>...>
    constexpr rvariant& operator=(rvariant<Us...> const& rhs)
    {
        detail::raw_visit(
            rhs.index_, // j
            [this, &rhs]<std::size_t j, class Uj>([[maybe_unused]] detail::alternative<j, Uj> const& rhs_alt) {
                if constexpr (j == std::variant_npos) {
                    if (index_ != std::variant_npos) {
                        dynamic_reset();
                    }
                } else { // rhs holds something
                    static constexpr std::size_t corresponding_i = subset_reindex_for<rvariant<Us...>>(j);
                    using VT = detail::select_maybe_wrapped_t<unwrap_recursive_t<Uj>, Ts...>;
                    static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<Uj>>);

                    detail::raw_visit(
                        index_,
                        [this, &rhs, &rhs_alt]<std::size_t i, class ThisAlt>([[maybe_unused]] detail::alternative<i, ThisAlt>& this_alt) {
                            if constexpr (i != std::variant_npos) { // rhs holds something, and *this holds something
                                if constexpr (std::is_same_v<unwrap_recursive_t<Uj>, unwrap_recursive_t<ThisAlt>>) {
                                    this_alt.value = detail::rewrap_maybe_recursive<ThisAlt>(rhs_alt.value);
                                } else {
                                    if constexpr (std::is_nothrow_copy_constructible_v<Uj> || !std::is_nothrow_move_constructible_v<Uj>) {
                                        emplace<corresponding_i>(detail::rewrap_maybe_recursive<VT>(rhs_alt.value));
                                    } else {
                                        this->operator=(rvariant<Us...>(rhs));
                                    }
                                }
                            } else { // rhs holds something, and *this is valueless
                                if constexpr (std::is_nothrow_copy_constructible_v<Uj> || !std::is_nothrow_move_constructible_v<Uj>) {
                                    emplace<corresponding_i>(detail::rewrap_maybe_recursive<VT>(rhs_alt.value));
                                } else {
                                    this->operator=(rvariant<Us...>(rhs));
                                }
                            }
                        },
                        *this
                    );
                }
            },
            rhs
        );
        return *this;
    }

    // Flexible move assignment operator
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            rvariant_set::subset_of<rvariant<Us...>, rvariant> &&
            std::conjunction_v<detail::variant_move_assignable<Us>...>
    constexpr rvariant& operator=(rvariant<Us...>&& rhs) noexcept(std::conjunction_v<detail::variant_nothrow_move_assignable<Us>...>)
    {
        detail::raw_visit(
            rhs.index_, // j
            [this, &rhs]<std::size_t j, class Uj>([[maybe_unused]] detail::alternative<j, Uj>&& rhs_alt) {
                if constexpr (j == std::variant_npos) {
                    if (index_ != std::variant_npos) {
                        dynamic_reset();
                    }
                } else { // rhs holds something
                    static constexpr std::size_t corresponding_i = subset_reindex_for<rvariant<Us...>>(j);
                    using VT = detail::select_maybe_wrapped_t<unwrap_recursive_t<Uj>, Ts...>;
                    static_assert(std::is_same_v<unwrap_recursive_t<VT>, unwrap_recursive_t<Uj>>);

                    detail::raw_visit(
                        index_,
                        [this, &rhs, &rhs_alt]<std::size_t i, class ThisAlt>([[maybe_unused]] detail::alternative<i, ThisAlt>& this_alt) {
                            if constexpr (i != std::variant_npos) { // rhs holds something, and *this holds something
                                if constexpr (std::is_same_v<unwrap_recursive_t<Uj>, unwrap_recursive_t<ThisAlt>>) {
                                    this_alt.value = detail::rewrap_maybe_recursive<ThisAlt>(std::move(rhs_alt).value);
                                } else {
                                    emplace<corresponding_i>(detail::rewrap_maybe_recursive<VT>(std::move(rhs_alt).value));
                                }
                            } else { // rhs holds something, and *this is valueless
                                emplace<corresponding_i>(detail::rewrap_maybe_recursive<VT>(std::move(rhs_alt).value));
                            }
                        },
                        *this
                    );
                }
            },
            std::move(rhs)
        );

        return *this;
    }

    // -------------------------------------------

    template<class T, class... Args>
        requires
            std::is_constructible_v<T, Args...> &&
            detail::non_wrapped_exactly_once_v<T, unwrapped_types>
    constexpr T& emplace(Args&&... args) YK_LIFETIMEBOUND // TODO: strengthen
    {
        constexpr std::size_t I = detail::find_index_v<T, unwrapped_types>;
        return emplace<I>(std::forward<Args>(args)...);
    }

    template<class T, class U, class... Args>
        requires
            std::is_constructible_v<T, std::initializer_list<U>&, Args...> &&
            detail::non_wrapped_exactly_once_v<T, unwrapped_types>
    constexpr T& emplace(std::initializer_list<U> il, Args&&... args) YK_LIFETIMEBOUND // TODO: strengthen
    {
        constexpr std::size_t I = detail::find_index_v<T, unwrapped_types>;
        return emplace<I>(il, std::forward<Args>(args)...);
    }

    template<std::size_t I, class... Args>
        requires std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, Args...>
    constexpr variant_alternative_t<I, rvariant>&
        emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<detail::pack_indexing_t<I, Ts...>, Args...>)
        YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        if (!valueless_by_exception()) dynamic_reset();
        std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...);
        index_ = I;
        return detail::unwrap_recursive(detail::raw_get<I>(*this).value);
    }

    template<std::size_t I, class U, class... Args>
        requires std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
    constexpr variant_alternative_t<I, rvariant>&
        emplace(std::initializer_list<U> il, Args&&... args) noexcept(std::is_nothrow_constructible_v<detail::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>)
        YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        if (!valueless_by_exception()) dynamic_reset();
        std::construct_at(&storage_, std::in_place_index<I>, il, std::forward<Args>(args)...);
        index_ = I;
        return detail::unwrap_recursive(detail::raw_get<I>(*this).value);
    }

    constexpr void swap(rvariant& rhs) noexcept(std::conjunction_v<detail::variant_nothrow_swappable<Ts>...>)
    {
        static_assert(std::conjunction_v<std::is_move_constructible<Ts>...>);
        static_assert(std::conjunction_v<std::is_swappable<Ts>...>);
        constexpr bool all_nothrow_swappable = std::conjunction_v<detail::variant_nothrow_swappable<Ts>...>;
        constexpr std::size_t instantiation_limit = 1024; // TODO: apply same logic to other visits with >= O(n^2) branch

        if constexpr (std::conjunction_v<detail::is_trivially_swappable<Ts>...>) {
            static_assert(detail::is_trivially_swappable_v<decltype(storage_)>);
            std::swap(storage_, rhs.storage_); // no ADL
            std::swap(index_, rhs.index_);

        } else if constexpr (sizeof...(Ts) * sizeof...(Ts) < instantiation_limit) {
            detail::raw_visit(
                index_,
                [this, &rhs]<std::size_t i, class ThisAlt>([[maybe_unused]] detail::alternative<i, ThisAlt>& this_alt) noexcept(all_nothrow_swappable) {
                    detail::raw_visit(
                        rhs.index_,
                        [this, &rhs, &this_alt]<std::size_t j, class RhsAlt>([[maybe_unused]] detail::alternative<j, RhsAlt>& rhs_alt)
                            noexcept(all_nothrow_swappable)
                        {
                            if constexpr (i == j) {
                                if constexpr (i != detail::variant_npos) {
                                    using std::swap;
                                    swap(this_alt.value, rhs_alt.value);
                                }

                            } else if constexpr (i == detail::variant_npos) {
                                this->template emplace_on_valueless<j>(std::move(rhs_alt.value));
                                rhs.template reset<j>();

                            } else if constexpr (j == detail::variant_npos) {
                                rhs.template emplace_on_valueless<i>(std::move(this_alt.value));
                                this->template reset<i>();

                            } else {
                                auto tmp = std::move(this_alt.value);
                                this->template reset<i>();
                                this->template emplace_on_valueless<j>(std::move(rhs_alt.value));
                                rhs.template reset<j>();
                                rhs.template emplace_on_valueless<i>(std::move(tmp));
                            }
                        },
                        rhs
                    );
                },
                *this
            );
        } else {
            if (index_ == rhs.index_) {
                detail::raw_visit(
                    rhs.index_,
                    [this]<std::size_t i, class RhsAlt>([[maybe_unused]] detail::alternative<i, RhsAlt>& rhs_alt)
                        noexcept(std::conjunction_v<std::is_nothrow_swappable<Ts>...>)
                    {
                        if constexpr (i != detail::variant_npos) {
                            using std::swap;
                            swap(detail::raw_get<i>(*this), rhs_alt.value);
                        }
                    },
                    rhs
                );
            } else {
                auto tmp = std::move(*this);
                this->dynamic_emplace_from(std::move(rhs));
                rhs.dynamic_emplace_from(std::move(tmp));
            }
        }
    }

    friend constexpr void swap(rvariant& v, rvariant& w)
        noexcept(noexcept(v.swap(w)))
        requires (std::conjunction_v<std::conjunction<std::is_move_constructible<Ts>, std::is_swappable<Ts>>...>)
    {
        v.swap(w);
    }

    [[nodiscard]] constexpr bool valueless_by_exception() const noexcept { return index_ == std::variant_npos; }
    [[nodiscard]] constexpr std::size_t index() const noexcept { return static_cast<std::size_t>(index_); }

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
        return detail::raw_visit(index_, detail::subset_visitor<rvariant, Us...>{}, *this);
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
        return detail::raw_visit(index_, detail::subset_visitor<rvariant, Us...>{}, std::move(*this));
    }

    template<class... Us>
    friend class rvariant;

    template<class Self, class... Us>
    friend struct detail::subset_visitor;

    template<std::size_t I, class Variant>
    friend constexpr auto&& detail::raw_get(Variant&&) noexcept;

    template<class Variant, class T>
    friend struct detail::exactly_once_index;

private:
    template<class From, class To>
    friend constexpr std::size_t detail::subset_reindex(std::size_t index) noexcept;

    template<class W>
    [[nodiscard]] static constexpr std::size_t subset_reindex_for(std::size_t index) noexcept
    {
        return detail::subset_reindex<W, rvariant>(index);
    }

    template<class T> requires std::is_same_v<T, detail::valueless_t> // hack: reduce compile error by half on unrelated overloads
    constexpr explicit rvariant(T const&) noexcept
        : storage_(detail::valueless)
        , index_(detail::variant_npos)
    {}

    template<std::size_t I, class... Args>
    constexpr variant_alternative_t<I, rvariant>&
        emplace_on_valueless(Args&&... args) noexcept(std::is_nothrow_constructible_v<detail::pack_indexing_t<I, Ts...>, Args...>)
        YK_LIFETIMEBOUND
    {
        assert(valueless_by_exception());
        std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...);
        index_ = I;
        return detail::unwrap_recursive(detail::raw_get<I>(*this).value);
    }

    constexpr void dynamic_emplace_from(rvariant&& rhs)
        noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
    {
        dynamic_reset();

        detail::raw_visit(
            rhs.index_,
            [this]<std::size_t i, class RhsAlt>([[maybe_unused]] detail::alternative<i, RhsAlt>&& rhs_alt)
                noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
            {
                if constexpr (i != detail::variant_npos) {
                    this->template emplace_on_valueless<i>(std::move(rhs_alt).value);
                }
            },
            std::move(rhs)
        );
    }

    constexpr void dynamic_destroy() noexcept
    {
        detail::raw_visit(
            index_,
            []<std::size_t i, class T>([[maybe_unused]] detail::alternative<i, T>& alt) noexcept {
                if constexpr (i != detail::variant_npos) {
                    alt.value.~T();
                }
            },
            *this
        );
    }

    template<std::size_t I>
    constexpr void destroy() noexcept
    {
        if constexpr (I != std::variant_npos) {
            using T = detail::pack_indexing_t<I, Ts...>;
            detail::raw_get<I>(*this).value.~T();
        }
    }

    constexpr void dynamic_reset() noexcept
    {
        dynamic_destroy();
        index_ = std::variant_npos;
    }

    template<std::size_t I>
    constexpr void reset() noexcept
    {
        if constexpr (I != std::variant_npos) {
            destroy<I>();
            index_ = std::variant_npos;
        }
    }

    detail::variant_storage_t<std::index_sequence_for<Ts...>, Ts...>
        storage_;

    detail::variant_index_t index_ = detail::variant_npos;
};

template<class T, class... Ts>
    requires detail::is_ttp_specialization_of_v<T, recursive_wrapper>
[[nodiscard]] constexpr bool holds_alternative(rvariant<Ts...> const& v) noexcept = delete;

template<class T, class... Ts>
[[nodiscard]] constexpr bool holds_alternative(rvariant<Ts...> const& v) noexcept
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    return v.index() == I;
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr variant_alternative_t<I, rvariant<Ts...>>&
get(rvariant<Ts...>& var YK_LIFETIMEBOUND)
{
    if (I != var.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(var).value);
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr variant_alternative_t<I, rvariant<Ts...>>&&
get(rvariant<Ts...>&& var YK_LIFETIMEBOUND)
{
    if (I != var.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(var)).value);
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr variant_alternative_t<I, rvariant<Ts...>> const&
get(rvariant<Ts...> const& var YK_LIFETIMEBOUND)
{
    if (I != var.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(var).value);
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr variant_alternative_t<I, rvariant<Ts...>> const&&
get(rvariant<Ts...> const&& var YK_LIFETIMEBOUND)
{
    if (I != var.index()) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(var)).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T&
get(rvariant<Ts...>& var YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (var.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(var).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T&&
get(rvariant<Ts...>&& var YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (var.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(var)).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T const&
get(rvariant<Ts...> const& var YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (var.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(var).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T const&&
get(rvariant<Ts...> const&& var YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    if (var.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(var)).value);
}

template<class T, class... Ts>
    requires detail::is_ttp_specialization_of_v<T, recursive_wrapper>
constexpr T& get(rvariant<Ts...>&) = delete;

template<class T, class... Ts>
    requires detail::is_ttp_specialization_of_v<T, recursive_wrapper>
constexpr T&& get(rvariant<Ts...>&&) = delete;

template<class T, class... Ts>
    requires detail::is_ttp_specialization_of_v<T, recursive_wrapper>
constexpr T const& get(rvariant<Ts...> const&) = delete;

template<class T, class... Ts>
    requires detail::is_ttp_specialization_of_v<T, recursive_wrapper>
constexpr T const&& get(rvariant<Ts...> const&&) = delete;

// ---------------------------------------------

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts...>>>
get_if(rvariant<Ts...>* v) noexcept
{
    if (v == nullptr || v->index() != I) return nullptr;
    return std::addressof(detail::unwrap_recursive(detail::raw_get<I>(*v).value));
}

template<std::size_t I, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<variant_alternative_t<I, rvariant<Ts...>> const>
get_if(rvariant<Ts...> const* v) noexcept
{
    if (v == nullptr || v->index() != I) return nullptr;
    return std::addressof(detail::unwrap_recursive(detail::raw_get<I>(*v).value));
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
get_if(rvariant<Ts...>* var) noexcept
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    return get_if<I>(var);
}

template<class T, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<T const>
get_if(rvariant<Ts...> const* var) noexcept
{
    constexpr std::size_t I = detail::exactly_once_index_v<T, rvariant<Ts...>>;
    return get_if<I>(var);
}

template<class T, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<T>
get(rvariant<Ts...>* var) noexcept
{
    return get_if<T>(var);
}

template<class T, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<T const>
get(rvariant<Ts...> const* var) noexcept
{
    return get_if<T>(var);
}

}  // namespace yk

#endif  // YK_RVARIANT_HPP
