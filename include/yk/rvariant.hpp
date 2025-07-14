#ifndef YK_RVARIANT_HPP
#define YK_RVARIANT_HPP

#include "yk/detail/exactly_once.hpp"
#include "yk/detail/is_specialization_of.hpp"
#include "yk/detail/pack_indexing.hpp"

#include <functional>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <variant>

#include <cstddef>

namespace yk {

template<class... Ts>
class rvariant;

template<class Variant>
struct variant_size;

template<class Variant>
inline constexpr std::size_t variant_size_v = variant_size<Variant>::value;

template<class Variant>
struct variant_size<Variant const> : variant_size<Variant> {};

template<class... Ts>
struct variant_size<rvariant<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template<std::size_t I, class Variant>
struct variant_alternative;

template<std::size_t I, class Variant>
using variant_alternative_t = typename variant_alternative<I, Variant>::type;

template<std::size_t I, class Variant>
struct variant_alternative<I, Variant const> : std::add_const<variant_alternative_t<I, Variant>> {};

template<std::size_t I, class... Ts>
struct variant_alternative<I, rvariant<Ts...>> : detail::pack_indexing<I, Ts...> {
    static_assert(I < sizeof...(Ts));
};

inline constexpr std::size_t variant_npos = -1;

namespace detail {

template<class... Ts>
concept all_destructible = std::conjunction_v<std::is_destructible<Ts>...>;

template<class... Ts>
concept all_trivially_destructible = all_destructible<Ts...> && std::conjunction_v<std::is_trivially_destructible<Ts>...>;

template<class... Ts>
concept all_copy_constructible = std::conjunction_v<std::is_copy_constructible<Ts>...>;

template<class... Ts>
concept all_trivially_copy_constructible = all_copy_constructible<Ts...> && std::conjunction_v<std::is_trivially_copy_constructible<Ts>...>;

template<class... Ts>
concept all_move_constructible = std::conjunction_v<std::is_move_constructible<Ts>...>;

template<class... Ts>
concept all_trivially_move_constructible = all_move_constructible<Ts...> && std::conjunction_v<std::is_trivially_move_constructible<Ts>...>;

template<class... Ts>
concept variant_copy_assignable = std::conjunction_v<std::conjunction<std::is_copy_constructible<Ts>, std::is_copy_assignable<Ts>>...>;

template<class... Ts>
concept variant_trivially_copy_assignable =
    variant_copy_assignable<Ts...> &&
    std::conjunction_v<
        std::conjunction<std::is_trivially_copy_constructible<Ts>, std::is_trivially_copy_assignable<Ts>, std::is_trivially_destructible<Ts>>...>;

template<class... Ts>
concept variant_move_assignable = std::conjunction_v<std::conjunction<std::is_move_constructible<Ts>, std::is_move_assignable<Ts>>...>;

template<class... Ts>
concept variant_trivially_move_assignable =
    variant_move_assignable<Ts...> &&
    std::conjunction_v<
        std::conjunction<std::is_trivially_move_constructible<Ts>, std::is_trivially_move_assignable<Ts>, std::is_trivially_destructible<Ts>>...>;

template<std::size_t I, class Dest, class Source>
struct fun_impl {
    using dest_array = Dest[];
    std::integral_constant<std::size_t, I> operator()(Dest arg)
        requires requires(Source source) { dest_array{std::forward<Source>(source)}; };
};

template<class Source, class Variant, class Seq = std::make_index_sequence<variant_size_v<Variant>>>
struct fun;

template<class Source, class Variant, std::size_t... Is>
struct fun<Source, Variant, std::index_sequence<Is...>> : fun_impl<Is, variant_alternative_t<Is, Variant>, Source>... {
    using fun_impl<Is, variant_alternative_t<Is, Variant>, Source>::operator()...;
};

template<class Source, class Variant>
using accepted_index = decltype(fun<Source, Variant>{}(std::declval<Source>()));

template<class Source, class Variant>
inline constexpr std::size_t accepted_index_v = accepted_index<Source, Variant>::value;

struct valueless_t {};

inline constexpr valueless_t valueless{};

template<bool TriviallyDestructible, class... Ts>
union variadic_union;

template<bool TriviallyDestructible>
union variadic_union<TriviallyDestructible> {
    constexpr variadic_union(valueless_t) {}
};

template<bool TriviallyDestructible, class T, class... Ts>
union variadic_union<TriviallyDestructible, T, Ts...> {
    constexpr variadic_union() : first() {}

    constexpr variadic_union(valueless_t) : rest(valueless) {}

    template<class... Args>
    constexpr variadic_union(std::in_place_index_t<0>, Args&&... args) : first(std::forward<Args>(args)...) {}

    template<std::size_t I, class... Args>
    constexpr variadic_union(std::in_place_index_t<I>, Args&&... args) : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...) {}

    ~variadic_union() = default;

    constexpr ~variadic_union()
        requires (!TriviallyDestructible)
    {}

    T first;
    variadic_union<TriviallyDestructible, Ts...> rest;
};

template<std::size_t I, class T>
struct alternative {
    static constexpr std::size_t index = I;
    using type = T;
    T value;
};

template<std::size_t I>
struct get_alternative {
    template<class Union>
    constexpr auto&& operator()(Union&& u) noexcept {
        return get_alternative<I - 1>{}(std::forward<Union>(u).rest);
    }
};

template<>
struct get_alternative<0> {
    template<class Union>
    constexpr auto&& operator()(Union&& u) noexcept {
        return std::forward<Union>(u).first;
    }
};

template<std::size_t I, class Variant>
constexpr auto&& raw_get(Variant&& var) noexcept {
    return get_alternative<I>{}(std::forward<Variant>(var).storage_);
}

template<std::size_t I, class Variant>
using alternative_t = decltype(raw_get<I>(std::declval<Variant>()));

template<class Visitor, class Variant>
using raw_visit_return_type = std::invoke_result_t<Visitor, alternative_t<0, Variant>>;

template<std::size_t I, class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant> raw_visit_dispatch(Visitor&& vis,
                                                                     Variant&& var) noexcept(std::is_nothrow_invocable_v<Visitor, alternative_t<0, Variant>>) {
    return std::invoke(std::forward<Visitor>(vis), raw_get<I>(std::forward<Variant>(var)));
}

template<class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant> raw_visit_valueless(Visitor&& vis, Variant&&) noexcept(
    std::is_nothrow_invocable_v<Visitor, alternative<variant_npos, std::monostate>>) {
    return std::invoke(std::forward<Visitor>(vis), alternative<variant_npos, std::monostate>{});
}

template<class Visitor, class Variant, class Seq = std::make_index_sequence<variant_size_v<std::remove_reference_t<Variant>>>>
struct raw_visit_dispatch_table;

template<class Visitor, class Variant, std::size_t... Is>
struct raw_visit_dispatch_table<Visitor, Variant, std::index_sequence<Is...>> {
    using function_type = raw_visit_return_type<Visitor, Variant> (*)(Visitor&&, Variant&&);
    static constexpr function_type value[] = {&raw_visit_valueless<Visitor, Variant>, &raw_visit_dispatch<Is, Visitor, Variant>...};
};

template<class Visitor, class Variant>
constexpr raw_visit_return_type<Visitor, Variant> raw_visit(std::size_t index, Visitor&& vis,
                                                            Variant&& var) noexcept(std::is_nothrow_invocable_v<Visitor, alternative_t<0, Variant>>) {
    constexpr auto const& table = raw_visit_dispatch_table<Visitor, Variant>::value;
    return std::invoke(table[index + 1], std::forward<Visitor>(vis), std::forward<Variant>(var));
}

template<class Seq, class... Ts>
struct variant_storage;

template<std::size_t... Is, class... Ts>
struct variant_storage<std::index_sequence<Is...>, Ts...> {
    using type = variadic_union<(std::is_trivially_destructible_v<Ts> && ...), alternative<Is, Ts>...>;
};

template<class Seq, class... Ts>
using variant_storage_t = typename variant_storage<Seq, Ts...>::type;

template<class...>
struct pack_union_helper {};

template<template<class...> class TT, class T, class... Us>
struct pack_union_impl;

template<template<class...> class TT, class... Ts>
struct pack_union_impl<TT, pack_union_helper<Ts...>> {
    using type = TT<Ts...>;
};

template<template<class...> class TT, class... Ts, class U, class... Us>
struct pack_union_impl<TT, pack_union_helper<Ts...>, U, Us...>
    : std::conditional_t<is_in_v<U, Ts...>, pack_union_impl<TT, pack_union_helper<Ts...>, Us...>, pack_union_impl<TT, pack_union_helper<Ts..., U>, Us...>> {};

template<template<class...> class TT, class A, class B>
struct pack_union;

template<template<class...> class TT, class A, class B>
struct pack_union : detail::pack_union_impl<TT, detail::pack_union_helper<>, A, B> {};

template<template<class...> class TT, class... As, class B>
struct pack_union<TT, TT<As...>, B> : detail::pack_union_impl<TT, detail::pack_union_helper<>, As..., B> {};

template<template<class...> class TT, class A, class... Bs>
struct pack_union<TT, A, TT<Bs...>> : detail::pack_union_impl<TT, detail::pack_union_helper<>, A, Bs...> {};

template<template<class...> class TT, class... As, class... Bs>
struct pack_union<TT, TT<As...>, TT<Bs...>> : detail::pack_union_impl<TT, detail::pack_union_helper<>, As..., Bs...> {};

template<template<class...> class TT, class A, class B>
using pack_union_t = typename pack_union<TT, A, B>::type;

template<class T>
struct unwrap_one_pack {
    using type = T;
};

template<template<class...> class TT, class T>
struct unwrap_one_pack<TT<T>> {
    using type = T;
};

}  // namespace detail

template<class T, class U>
struct is_subset_of : std::false_type {};

template<template<class...> class L1, class... Ts, template<class...> class L2, class... Us>
struct is_subset_of<L1<Ts...>, L2<Us...>> : std::conjunction<detail::is_in<Ts, Us...>...> {};

template<class T, class U>
inline constexpr bool is_subset_of_v = is_subset_of<T, U>::value;

template<class T, class U>
concept subset_of = is_subset_of_v<T, U>;

template<class T, class U>
concept equivalent_to = subset_of<T, U> && subset_of<U, T>;

template<template<class...> class TT, class A, class B>
struct compact_alternative : detail::unwrap_one_pack<detail::pack_union_t<TT, A, B>> {};

template<template<class...> class TT, class A, class B>
using compact_alternative_t = typename compact_alternative<TT, A, B>::type;

template<class... Ts>
class rvariant {
private:
    class variant_npos_setter {
    public:
        constexpr variant_npos_setter(rvariant* par) noexcept : parent_(par) {}

        constexpr void mark_succeeded() noexcept { succeeded_ = true; }

        constexpr ~variant_npos_setter() noexcept {
            if (!succeeded_) parent_->index_ = variant_npos;
        }

    private:
        rvariant* parent_;
        bool succeeded_ = false;
    };

    friend struct variant_npos_setter;

public:
    static_assert((std::is_destructible_v<Ts> && ...));
    static_assert(sizeof...(Ts) > 0);

    constexpr rvariant() noexcept(std::is_nothrow_default_constructible_v<detail::pack_indexing_t<0, Ts...>>)
        requires std::is_default_constructible_v<detail::pack_indexing_t<0, Ts...>>
        : storage_{}, index_(0) {}

    constexpr rvariant(rvariant const&)
        requires detail::all_trivially_copy_constructible<Ts...>
    = default;

    constexpr rvariant(rvariant const&)
        requires (!detail::all_copy_constructible<Ts...>)
    = delete;

    constexpr rvariant(rvariant const& other)
        requires detail::all_copy_constructible<Ts...>
        : storage_(detail::valueless), index_(other.index_) {
        detail::raw_visit(
            index_,
            [this]<std::size_t I, class T>(detail::alternative<I, T> const& alt) {
                if constexpr (I != variant_npos) {
                    std::construct_at(&storage_, std::in_place_index<I>, alt.value);
                }
            },
            other);
    }

    constexpr rvariant(rvariant&&)
        requires detail::all_trivially_move_constructible<Ts...>
    = default;

    constexpr rvariant(rvariant&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
        requires detail::all_move_constructible<Ts...>
        : storage_(detail::valueless), index_(other.index_) {
        variant_npos_setter guard(this);
        detail::raw_visit(
            index_,
            [this]<std::size_t I, class T>(detail::alternative<I, T>&& alt) {
                if constexpr (I != variant_npos) {
                    std::construct_at(&storage_, std::in_place_index<I>, std::move(alt).value);
                }
            },
            std::move(other));
        guard.mark_succeeded();
    }

    template<class... Us>
        requires (!(detail::all_copy_constructible<Us...> && subset_of<rvariant<Us...>, rvariant>))
    constexpr rvariant(rvariant<Us...> const&) = delete;

    template<class... Us>
        requires detail::all_copy_constructible<Us...> && subset_of<rvariant<Us...>, rvariant>
    constexpr rvariant(rvariant<Us...> const& other) : storage_(detail::valueless), index_(detail::convert_index<rvariant<Us...>, rvariant>(other.index_)) {
        detail::raw_visit(
            other.index_,
            [this]<std::size_t I, class T>(detail::alternative<I, T> const& alt) {
                if constexpr (I != variant_npos) {
                    std::construct_at(&storage_, std::in_place_index<detail::convert_index<rvariant<Us...>, rvariant>(I)>, alt.value);
                }
            },
            other);
    }

    template<class... Us>
        requires (!(detail::all_move_constructible<Us...> && subset_of<rvariant<Us...>, rvariant>))
    constexpr rvariant(rvariant<Us...>&&) = delete;

    template<class... Us>
        requires detail::all_move_constructible<Us...> && subset_of<rvariant<Us...>, rvariant>
    constexpr rvariant(rvariant<Us...>&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Us>...>)
        : storage_(detail::valueless), index_(detail::convert_index<rvariant<Us...>, rvariant>(other.index_)) {
        variant_npos_setter guard(this);
        detail::raw_visit(
            other.index_,
            [this]<std::size_t I, class T>(detail::alternative<I, T>&& alt) {
                if constexpr (I != variant_npos) {
                    std::construct_at(&storage_, std::in_place_index<detail::convert_index<rvariant<Us...>, rvariant>(I)>, std::move(alt).value);
                }
            },
            std::move(other));
        guard.mark_succeeded();
    }

    template<class T>
        requires requires {
            requires sizeof...(Ts) > 0;
            requires !detail::is_ttp_specialization_of_v<std::remove_cvref_t<T>, rvariant>;
            requires !detail::is_ttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_type_t>;
            requires !detail::is_nttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_index_t>;
            requires requires(T source) { detail::fun<T, rvariant>{}(std::forward<T>(source)); };
            requires std::is_constructible_v<detail::pack_indexing_t<detail::accepted_index_v<T, rvariant>, Ts...>, T>;
        }
    constexpr rvariant(T&& x) noexcept(std::is_nothrow_constructible_v<detail::pack_indexing_t<detail::accepted_index_v<T, rvariant>, Ts...>, T>)
        : rvariant(std::in_place_index<detail::accepted_index_v<T, rvariant>>, std::forward<T>(x)) {}

    template<class T, class... Args>
        requires detail::exactly_once_v<T, Ts...> && std::is_constructible_v<T, Args...>
    constexpr explicit rvariant(std::in_place_type_t<T>, Args&&... args)
        : rvariant(std::in_place_index<detail::accepted_index_v<T, rvariant>>, std::forward<Args>(args)...) {}

    template<class T, class U, class... Args>
        requires detail::exactly_once_v<T, Ts...> && std::is_constructible_v<T, std::initializer_list<U>&, Args...>
    constexpr explicit rvariant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
        : rvariant(std::in_place_index<detail::accepted_index_v<T, rvariant>>, il, std::forward<Args>(args)...) {}

    template<std::size_t I, class... Args>
        requires (I < sizeof...(Ts)) && std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, Args...>
    constexpr explicit rvariant(std::in_place_index_t<I>, Args&&... args) : storage_(std::in_place_index<I>, std::forward<Args>(args)...), index_(I) {}

    template<std::size_t I, class U, class... Args>
        requires (I < sizeof...(Ts)) && std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
    constexpr explicit rvariant(std::in_place_index_t<I>, std::initializer_list<U> il, Args&&... args)
        : storage_(std::in_place_index<I>, il, std::forward<Args>(args)...), index_(I) {}

private:
    constexpr void destroy() {
        detail::raw_visit(
            index_,
            []<class Alt>(Alt&& alt) {
                if constexpr (std::remove_cvref_t<Alt>::index != variant_npos) {
                    using T = typename std::remove_cvref_t<Alt>::type;
                    alt.value.~T();
                }
            },
            *this);
    }

    constexpr void reset() {
        destroy();
        index_ = variant_npos;
    }

    constexpr rvariant& self() { return *this; }

public:
    constexpr rvariant& operator=(rvariant const&)
        requires detail::variant_trivially_copy_assignable<Ts...>
    = default;

    constexpr rvariant& operator=(rvariant const&)
        requires (!detail::variant_copy_assignable<Ts...>)
    = delete;

    constexpr rvariant& operator=(rvariant const& other)
        requires detail::variant_copy_assignable<Ts...>
    {
        detail::raw_visit(
            other.index_,
            [this]<class Alt>(Alt&& alt) {
                constexpr std::size_t I = std::remove_cvref_t<Alt>::index;
                using T = std::remove_cvref_t<Alt>::type;
                if constexpr (I == variant_npos) {
                    reset();
                } else {
                    if (index_ == I) {
                        // directly assign
                        detail::raw_get<I>(self()).value = alt.value;
                    } else {
                        if constexpr (std::is_nothrow_copy_constructible_v<T> || !std::is_nothrow_move_constructible_v<T>) {
                            // copy is nothrow or move throws; use copy constructor
                            reset();
                            std::construct_at(&storage_, std::in_place_index<I>, alt.value);
                        } else {
                            // copy throws and move is nothrow; move temporary copy
                            auto temporary = alt.value;
                            reset();
                            std::construct_at(&storage_, std::in_place_index<I>, std::move(temporary));
                        }
                        index_ = I;
                    }
                }
            },
            other);
        return *this;
    }

    constexpr rvariant& operator=(rvariant&&)
        requires detail::variant_trivially_move_assignable<Ts...>
    = default;

    constexpr rvariant& operator=(rvariant&& other) noexcept(
        std::conjunction_v<std::conjunction<std::is_nothrow_move_constructible<Ts>, std::is_nothrow_move_assignable<Ts>>...>)
        requires detail::variant_move_assignable<Ts...>
    {
        detail::raw_visit(
            other.index_,
            [this]<class Alt>(Alt&& alt) {
                constexpr std::size_t I = std::remove_cvref_t<Alt>::index;
                if constexpr (I == variant_npos) {
                    reset();
                } else {
                    if (index_ == I) {
                        detail::raw_get<I>(self()).value = std::move(alt).value;
                    } else {
                        reset();
                        std::construct_at(&storage_, std::in_place_index<I>, std::move(alt).value);
                        index_ = I;
                    }
                }
            },
            other);
        return *this;
    }

    constexpr ~rvariant() = default;

    constexpr ~rvariant()
        requires (!std::conjunction_v<std::is_trivially_destructible<Ts>...>)
    {
        reset();
    }

    constexpr bool valueless_by_exception() const noexcept { return index_ == variant_npos; }
    constexpr std::size_t index() const noexcept { return index_; }

private:
    constexpr rvariant(detail::valueless_t) noexcept : storage_(detail::valueless), index_(variant_npos) {}

    template<class... Us>
    static constexpr auto subset_visitor = []<class Alt>(Alt&& alt) -> rvariant<Us...> /* noexcept is TODO */ {
        if constexpr (std::remove_cvref_t<Alt>::index == variant_npos) {
            return rvariant<Us...>(detail::valueless);
        } else {
            constexpr std::size_t pos = detail::convert_index<rvariant, rvariant<Us...>>(std::remove_cvref_t<Alt>::index);
            if constexpr (pos == detail::find_index_npos) {
                throw std::bad_variant_access{};
            } else {
                return rvariant<Us...>(std::in_place_index<pos>, std::forward<Alt>(alt).value);
            }
        }
    };

public:
    template<class... Us>
        requires std::same_as<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant subset() const& noexcept(std::is_nothrow_copy_constructible_v<rvariant>) {
        return *this;
    }

    template<class... Us>
        requires std::same_as<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant subset() && noexcept(std::is_nothrow_move_constructible_v<rvariant>) {
        return std::move(*this);
    }

    template<class... Us>
        requires (!std::same_as<rvariant<Us...>, rvariant>) && subset_of<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant<Us...> subset() const& noexcept(equivalent_to<rvariant<Us...>, rvariant> &&
                                                                     std::is_nothrow_copy_constructible_v<rvariant>) {
        return detail::raw_visit(index_, subset_visitor<Us...>, *this);
    }

    template<class... Us>
        requires (!std::same_as<rvariant<Us...>, rvariant>) && subset_of<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant<Us...> subset() && noexcept(equivalent_to<rvariant<Us...>, rvariant> && std::is_nothrow_move_constructible_v<rvariant>) {
        return detail::raw_visit(index_, subset_visitor<Us...>, std::move(*this));
    }

    template<class... Us>
    friend class rvariant;

    template<std::size_t I, class Variant>
    friend constexpr auto&& detail::raw_get(Variant&&) noexcept;

private:
    detail::variant_storage_t<std::index_sequence_for<Ts...>, Ts...> storage_;
    std::size_t index_ = variant_npos;
};

template<std::size_t I, class... Ts>
constexpr variant_alternative_t<I, rvariant<Ts...>>& get(rvariant<Ts...>& var) {
    if (I != var.index()) throw std::bad_variant_access{};
    return detail::raw_get<I>(var).value;
}

template<std::size_t I, class... Ts>
constexpr variant_alternative_t<I, rvariant<Ts...>>&& get(rvariant<Ts...>&& var) {
    if (I != var.index()) throw std::bad_variant_access{};
    return detail::raw_get<I>(std::move(var)).value;
}

template<std::size_t I, class... Ts>
constexpr variant_alternative_t<I, rvariant<Ts...>> const& get(rvariant<Ts...> const& var) {
    if (I != var.index()) throw std::bad_variant_access{};
    return detail::raw_get<I>(var).value;
}

template<std::size_t I, class... Ts>
constexpr variant_alternative_t<I, rvariant<Ts...>> const&& get(rvariant<Ts...> const&& var) {
    if (I != var.index()) throw std::bad_variant_access{};
    return detail::raw_get<I>(std::move(var)).value;
}

}  // namespace yk

#endif  // YK_RVARIANT_HPP
