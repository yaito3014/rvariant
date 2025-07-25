#ifndef YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP
#define YK_RVARIANT_DETAIL_VARIANT_STORAGE_HPP

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/variant_helper.hpp>
#include <yk/core/seq.hpp>

#include <variant> // std::bad_variant_access
#include <functional>
#include <utility>
#include <type_traits>

namespace yk::detail {

template<bool TriviallyDestructible, class... Ts>
struct variadic_union {};

template<class... Ts>
using make_variadic_union_t = variadic_union<std::conjunction_v<std::is_trivially_destructible<Ts>...>, Ts...>;


template<bool NeverValueless>
[[nodiscard]] constexpr std::size_t valueless_bias(std::size_t i) noexcept
{
    if constexpr (NeverValueless) {
        return i;
    } else {
        return i + 1;
    }
}

template<bool NeverValueless>
[[nodiscard]] constexpr std::size_t valueless_unbias(std::size_t i) noexcept
{
    if constexpr (NeverValueless) {
        return i;
    } else {
        return i - 1;
    }
}

template<class... Ts>
struct is_never_valueless : std::false_type {}; // TODO

template<class... Ts>
constexpr bool is_never_valueless_v = is_never_valueless<Ts...>::value;


template<class T, class... Ts>
struct variadic_union<true, T, Ts...>
{
    static_assert(true == std::conjunction_v<std::is_trivially_destructible<T>, std::is_trivially_destructible<Ts>...>);

    static constexpr std::size_t size = sizeof...(Ts) + 1;
    static constexpr bool never_valueless = is_never_valueless_v<Ts...>;

    // no active member
    // ReSharper disable once CppPossiblyUninitializedMember
    constexpr explicit variadic_union() noexcept {}

#ifdef __RESHARPER__
    // These are required for propagating traits
    variadic_union(variadic_union const&) = default;
    variadic_union(variadic_union&&) = default;
    variadic_union& operator=(variadic_union const&) = default;
    variadic_union& operator=(variadic_union&&) = default;

    // According to the standard, a union with non-trivially-(copy|move)-(constructible|assignable)
    // members has *implicitly*-deleted corresponding special functions.
    // Although it should work only by the "= default" declaration, some compilers
    // (e.g. ReSharper's "Code Inspection") fail to detect such traits,
    // resulting in red squiggles everywhere.
    variadic_union(variadic_union const&)            requires((!std::conjunction_v<std::is_trivially_copy_constructible<T>, std::is_trivially_copy_constructible<Ts>...>)) = delete;
    variadic_union(variadic_union&&)                 requires((!std::conjunction_v<std::is_trivially_move_constructible<T>, std::is_trivially_move_constructible<Ts>...>)) = delete;
    variadic_union& operator=(variadic_union const&) requires((!std::conjunction_v<std::is_trivially_copy_assignable<T>, std::is_trivially_copy_assignable<Ts>...>)) = delete;
    variadic_union& operator=(variadic_union&&)      requires((!std::conjunction_v<std::is_trivially_move_assignable<T>, std::is_trivially_move_assignable<Ts>...>)) = delete;
#endif

    template<class... Args>
        requires std::is_constructible_v<T, Args...> // required for not confusing some compilers
    constexpr explicit variadic_union(std::in_place_index_t<0>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : first(std::forward<Args>(args)...) // value-initialize; https://eel.is/c++draft/variant.ctor#3
    {}

    template<std::size_t I, class... Args>
        requires (I != 0) && std::is_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>
    constexpr explicit variadic_union(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>)
        : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...)
    {}

    union {
        T first;
        make_variadic_union_t<Ts...> rest;
    };
};

template<class T, class... Ts>
struct variadic_union<false, T, Ts...>
{
    static_assert(false == std::conjunction_v<std::is_trivially_destructible<T>, std::is_trivially_destructible<Ts>...>);

    static constexpr std::size_t size = sizeof...(Ts) + 1;
    static constexpr bool never_valueless = is_never_valueless_v<Ts...>;

    // no active member
    // ReSharper disable once CppPossiblyUninitializedMember
    constexpr explicit variadic_union() noexcept {}

    constexpr ~variadic_union() noexcept {}

    variadic_union(variadic_union const&) = default;
    variadic_union(variadic_union&&) = default;
    variadic_union& operator=(variadic_union const&) = default;
    variadic_union& operator=(variadic_union&&) = default;

#ifdef __RESHARPER__
    variadic_union(variadic_union const&)            requires((!std::conjunction_v<std::is_trivially_copy_constructible<T>, std::is_trivially_copy_constructible<Ts>...>)) = delete;
    variadic_union(variadic_union&&)                 requires((!std::conjunction_v<std::is_trivially_move_constructible<T>, std::is_trivially_move_constructible<Ts>...>)) = delete;
    variadic_union& operator=(variadic_union const&) requires((!std::conjunction_v<std::is_trivially_copy_assignable<T>, std::is_trivially_copy_assignable<Ts>...>)) = delete;
    variadic_union& operator=(variadic_union&&)      requires((!std::conjunction_v<std::is_trivially_move_assignable<T>, std::is_trivially_move_assignable<Ts>...>)) = delete;
#endif

    template<class... Args>
        requires std::is_constructible_v<T, Args...> // required for not confusing some compilers
    constexpr explicit variadic_union(std::in_place_index_t<0>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : first(std::forward<Args>(args)...) // value-initialize; https://eel.is/c++draft/variant.ctor#3
    {}

    template<std::size_t I, class... Args>
        requires (I != 0) && std::is_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>
    constexpr explicit variadic_union(std::in_place_index_t<I>, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<make_variadic_union_t<Ts...>, std::in_place_index_t<I - 1>, Args...>)
        : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...)
    {}

    union {
        T first;
        make_variadic_union_t<Ts...> rest;
    };
};

// TODO: apply same logic to other visits with >= O(n^2) branch
inline constexpr std::size_t visit_instantiation_limit = 1024;

template<std::size_t I>
struct get_alternative
{
    static_assert(I != variant_npos);

    template<class Union>
    static constexpr auto&& apply(Union&& u YK_LIFETIMEBOUND) noexcept
    {
        return get_alternative<I - 1>::apply(std::forward<Union>(u).rest);
    }
};

template<>
struct get_alternative<0>
{
    template<class Union>
    static constexpr auto&& apply(Union&& u YK_LIFETIMEBOUND) noexcept
    {
        return std::forward<Union>(u).first;
    }
};

template<std::size_t I, class Storage>
[[nodiscard]] constexpr auto&& raw_get(Storage&& storage YK_LIFETIMEBOUND) noexcept
{
    return get_alternative<I>::apply(std::forward<Storage>(storage));
}

template<std::size_t I, class Storage>
using raw_get_t = decltype(raw_get<I>(std::declval<Storage>()));


template<class Visitor, class Storage, class Is = std::make_index_sequence<std::remove_cvref_t<Storage>::size>>
struct raw_visit_return_type_impl;

template<class Visitor, class Storage, std::size_t I>
struct raw_visit_return_type_impl<Visitor, Storage, std::index_sequence<I>>
{
    static_assert(
        std::is_invocable_v<Visitor, std::in_place_index_t<I>, raw_get_t<I, Storage>>,
        "The spec mandates that the Visitor accept all combinations of alternative types "
        "(https://eel.is/c++draft/variant.visit#5)."
    );

    using type = std::invoke_result_t<Visitor, std::in_place_index_t<I>, raw_get_t<I, Storage>>;
};

template<class Visitor, class Storage, std::size_t I, std::size_t... Is> requires (sizeof...(Is) > 0)
struct raw_visit_return_type_impl<Visitor, Storage, std::index_sequence<I, Is...>>
    : raw_visit_return_type_impl<Visitor, Storage, std::index_sequence<Is...>>
{
    static_assert(
        std::conjunction_v<std::is_same<
            std::invoke_result_t<Visitor, std::in_place_index_t<I>, raw_get_t<I, Storage>>,
            std::invoke_result_t<Visitor, std::in_place_index_t<Is>, raw_get_t<Is, Storage>>
        >...>,
        "The spec mandates that the Visitor return the same type and value category "
        "for all combinations of alternative types (https://eel.is/c++draft/variant.visit#5)."
    );
};

template<class Visitor, class Storage>
using raw_visit_return_type = typename raw_visit_return_type_impl<Visitor, Storage>::type;


template<class Visitor, class Storage>
constexpr raw_visit_return_type<Visitor, Storage>
raw_visit_valueless(Visitor&& vis, Storage&&)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    noexcept(std::is_nothrow_invocable_v<Visitor, std::in_place_index_t<variant_npos>, decltype(std::forward_like<Storage>(std::declval<valueless_t>()))>)
{
    valueless_t valueless_;
    return std::invoke(std::forward<Visitor>(vis), std::in_place_index<variant_npos>, std::forward_like<Storage>(valueless_));
}

template<std::size_t I, class Visitor, class Storage>
constexpr raw_visit_return_type<Visitor, Storage>
raw_visit_dispatch(Visitor&& vis, Storage&& storage)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    noexcept(std::is_nothrow_invocable_v<Visitor, std::in_place_index_t<I>, raw_get_t<I, Storage>>)
{
    return std::invoke(std::forward<Visitor>(vis), std::in_place_index<I>, raw_get<I>(std::forward<Storage>(storage)));
}

template<class Visitor, class Storage, class Is = std::make_index_sequence<std::remove_cvref_t<Storage>::size>>
constexpr bool raw_visit_noexcept = false;

template<class Visitor, class Storage, std::size_t... Is>
constexpr bool raw_visit_noexcept<Visitor, Storage, std::index_sequence<Is...>> = std::conjunction_v<
    std::is_nothrow_invocable<Visitor, std::in_place_index_t<variant_npos>, decltype(std::forward_like<Storage>(std::declval<valueless_t>()))>,
    std::is_nothrow_invocable<Visitor, std::in_place_index_t<Is>, raw_get_t<Is, Storage>>...
>;


template<class Visitor, class Storage, class Seq = std::make_index_sequence<std::remove_cvref_t<Storage>::size>>
struct raw_visit_dispatch_table;

template<class Visitor, class Storage>
using raw_visit_function_ptr = raw_visit_return_type<Visitor, Storage>(*) (Visitor&&, Storage&&)
noexcept(raw_visit_noexcept<Visitor, Storage>);

template<class Visitor, class Storage, std::size_t... Is>
struct raw_visit_dispatch_table<Visitor, Storage, std::index_sequence<Is...>>
{
    static_assert(std::is_reference_v<Visitor>, "Visitor must be one of: &, const&, &&, const&&");
    static_assert(std::is_reference_v<Storage>, "Storage must be one of: &, const&, &&, const&&");

    static constexpr raw_visit_function_ptr<Visitor, Storage> value[] = {
        &raw_visit_valueless<Visitor, Storage>,
        &raw_visit_dispatch<Is, Visitor, Storage>...
    };
};

// --------------------------------------------------
// --------------------------------------------------
// --------------------------------------------------

// https://eel.is/c++draft/variant.visit

namespace as_variant_impl {

template<class... Ts>
constexpr auto&& as_variant(rvariant<Ts...>& var) { return var; }

template<class... Ts>
constexpr auto&& as_variant(rvariant<Ts...> const& var) { return var; }

template<class... Ts>
constexpr auto&& as_variant(rvariant<Ts...>&& var) { return std::move(var); }

template<class... Ts>
constexpr auto&& as_variant(rvariant<Ts...> const&& var) { return std::move(var); }

} // as_variant_impl

template<class T>
using as_variant_t = decltype(as_variant_impl::as_variant(std::declval<T>()));

// --------------------------------------------------

template<class Variant>
struct forward_storage_t_impl
{
    static_assert(
        core::is_ttp_specialization_of_v<std::remove_cvref_t<Variant>, rvariant>,
        "`forward_storage` will only accept types which are exactly `rvariant`. Maybe you forgot `as_rvariant_t`?"
    );
    using type = decltype(std::declval<Variant>().storage());
};

template<class Variant>
using forward_storage_t = typename forward_storage_t_impl<Variant>::type;

template<class Variant>
[[nodiscard]] constexpr forward_storage_t<Variant>&&
forward_storage(std::remove_reference_t<Variant>& v YK_LIFETIMEBOUND) noexcept
{
    return std::forward<Variant>(v).storage();
}

template<class Variant>
[[nodiscard]] constexpr forward_storage_t<Variant>&&
forward_storage(std::remove_reference_t<Variant>&& v YK_LIFETIMEBOUND) noexcept  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
{
    return std::forward<Variant>(v).storage();
}


// `std::invoke_result_t` MUST NOT be used here due to its side effects:
// <https://eel.is/c++draft/meta.trans.other#tab:meta.trans.other-row-11-column-2-note-2>
// In the case of `variant`, this is not a theoretical concern:
// it has observable consequences where `visit(...)` may be incorrectly
// instantiated during the invocation of `visit<R>(...)`.
// This likely explains why [variant.visit](https://eel.is/c++draft/variant.visit#6)
// explicitly requires using `decltype(e(m))` instead of `std::invoke_result_t`.
// Also, we can't wrap this into a `struct` as it becomes not SFINAE-friendly.

template<class Visitor, class... Variants>
using visit_result_t = decltype(std::invoke( // If you see an error here, your `T0` is not eligible for the `Visitor`.
    std::declval<Visitor>(),
    detail::raw_get<0>(forward_storage<Variants>(std::declval<Variants>()))...
));

template<class T0R, class Visitor, class ArgsList, class... Variants>
struct visit_check_impl;

template<class T0R, class Visitor, class... Args>
struct visit_check_impl<T0R, Visitor, core::type_list<Args...>>
{
    static constexpr bool accepts_all_alternatives = std::is_invocable_v<Visitor, Args...>;

    template<class Visitor_, class... Args_>
    struct lazy_invoke
    {
        using type = decltype(std::invoke(std::declval<Visitor_>(), std::declval<Args_>()...));
    };

    // In case of `accepts_all_alternatives == false`, this
    // intentionally reports false-positive `true` to avoid
    // two `static_assert` errors.
    static constexpr bool same_return_type = std::is_same_v<
        typename std::conditional_t<
            accepts_all_alternatives,
            lazy_invoke<Visitor, Args...>,
            std::type_identity<T0R>
        >::type,
        T0R
    >;

    // for short-circuiting on conjunction
    static constexpr bool value = accepts_all_alternatives && same_return_type;
};

template<class T0R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_check_impl<T0R, Visitor, core::type_list<Args...>, rvariant<Ts...>&, Rest...>
    : std::conjunction<visit_check_impl<T0R, Visitor, core::type_list<Args..., Ts&>, Rest...>...> {};
template<class T0R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_check_impl<T0R, Visitor, core::type_list<Args...>, rvariant<Ts...> const&, Rest...>
    : std::conjunction<visit_check_impl<T0R, Visitor, core::type_list<Args..., Ts const&>, Rest...>...> {};
template<class T0R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_check_impl<T0R, Visitor, core::type_list<Args...>, rvariant<Ts...>&&, Rest...>
    : std::conjunction<visit_check_impl<T0R, Visitor, core::type_list<Args..., Ts>, Rest...>...> {};
template<class T0R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_check_impl<T0R, Visitor, core::type_list<Args...>, rvariant<Ts...> const&&, Rest...>
    : std::conjunction<visit_check_impl<T0R, Visitor, core::type_list<Args..., Ts const>, Rest...>...> {};

template<class T0R, class Visitor, class... Variants>
using visit_check = visit_check_impl<T0R, Visitor, core::type_list<>, Variants...>;


template<class R, class Visitor, class ArgsList, class... Variants>
struct visit_R_check_impl;

template<class R, class Visitor, class... Args>
struct visit_R_check_impl<R, Visitor, core::type_list<Args...>>
{
    // Note that this can't be `std::is_invocable_r_v`,
    // because we make sure the conditions for `static_assert` be
    // mutually exclusive in order to provide better errors.
    static constexpr bool accepts_all_alternatives = std::is_invocable_v<Visitor, Args...>;

    template<class Visitor_, class... Args_>
    struct lazy_invoke
    {
        // This is NOT `std::invoke_r`; we need the plain type for conversion check
        using type = decltype(std::invoke(std::declval<Visitor_>(), std::declval<Args_>()...));
    };

    // In case of `accepts_all_alternatives == false`, this
    // intentionally reports false-positive `true` to avoid
    // two `static_assert` errors.
    static constexpr bool return_type_convertible_to_R = std::is_convertible_v<
        typename std::conditional_t<
            accepts_all_alternatives,
            lazy_invoke<Visitor, Args...>,
            std::type_identity<R>
        >::type,
        R
    >;

    // for short-circuiting on conjunction
    static constexpr bool value = accepts_all_alternatives && return_type_convertible_to_R;
};

template<class R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_R_check_impl<R, Visitor, core::type_list<Args...>, rvariant<Ts...>&, Rest...>
    : std::conjunction<visit_R_check_impl<R, Visitor, core::type_list<Args..., Ts&>, Rest...>...> {};
template<class R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_R_check_impl<R, Visitor, core::type_list<Args...>, rvariant<Ts...> const&, Rest...>
    : std::conjunction<visit_R_check_impl<R, Visitor, core::type_list<Args..., Ts const&>, Rest...>...> {};
template<class R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_R_check_impl<R, Visitor, core::type_list<Args...>, rvariant<Ts...>&&, Rest...>
    : std::conjunction<visit_R_check_impl<R, Visitor, core::type_list<Args..., Ts>, Rest...>...> {};
template<class R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_R_check_impl<R, Visitor, core::type_list<Args...>, rvariant<Ts...> const&&, Rest...>
    : std::conjunction<visit_R_check_impl<R, Visitor, core::type_list<Args..., Ts const>, Rest...>...> {};

template<class R, class Visitor, class... Variants>
using visit_R_check = visit_R_check_impl<R, Visitor, core::type_list<>, Variants...>;


// --------------------------------------------------

template<class R, class Visitor, class OverloadSeq, class... Storage>
struct multi_visitor;

template<class R, class Visitor, std::size_t... Is, class... Storage>
struct multi_visitor<R, Visitor, std::index_sequence<Is...>, Storage...>
{
    static_assert(sizeof...(Is) == sizeof...(Storage));

    static constexpr R apply([[maybe_unused]] Visitor&& vis, [[maybe_unused]] Storage&&... storage)
    {
        if constexpr (((!std::remove_cvref_t<Storage>::never_valueless && Is == 0) || ...)) {
            throw std::bad_variant_access{};
        } else {
            return std::invoke_r<R>(
                std::forward<Visitor>(vis),
                raw_get<valueless_unbias<std::remove_cvref_t<Storage>::never_valueless>(Is)>(std::forward<Storage>(storage))...
            );
        }
    }
};

// --------------------------------------------------

template<class R, class Visitor, class Storage, class OverloadSeq>
struct visit_table;

template<class R, class Visitor, class... Storage, class... OverloadSeq>
struct visit_table<
    R,
    Visitor,
    core::type_list<Storage...>,
    core::type_list<OverloadSeq...>
>
{
    using function_type = R(*)(Visitor&&, Storage&&...);

    static constexpr function_type table[] = {
        &multi_visitor<R, Visitor, OverloadSeq, Storage...>::apply...
    };
};

// --------------------------------------------------

template<class Ns, class NeverValuelessMap>
struct flat_index;

template<std::size_t... Ns, bool... NeverValueless>
struct flat_index<std::index_sequence<Ns...>, std::integer_sequence<bool, NeverValueless...>>
{
    template<class... RuntimeIndex>
    [[nodiscard]] static constexpr std::size_t
    get(RuntimeIndex... index) noexcept
    {
        static_assert(sizeof...(RuntimeIndex) == sizeof...(Ns));
        return flat_index::get_impl(strides, index...);
    }

private:
    template<std::size_t... Stride, class... RuntimeIndex>
    [[nodiscard]] static constexpr std::size_t
    get_impl(std::index_sequence<Stride...>, RuntimeIndex... index) noexcept
    {
        return ((Stride * valueless_bias<NeverValueless>(index)) + ... + 0);
    }

    template<std::size_t TheI, std::size_t i, std::size_t... i_rest>
    [[nodiscard]] static consteval std::size_t
    calc_single_stride(
        std::index_sequence<i, i_rest...>,
        std::size_t const ofs = 1
    ) noexcept
    {
        static constexpr std::size_t Max = sizeof...(Ns) - 1;
        constexpr std::size_t factor = (Max - i) == TheI ? 1 : 0;

        if constexpr (sizeof...(i_rest) > 0) {
            return ofs * factor + calc_single_stride<TheI>(
                std::index_sequence<i_rest...>{},
                ofs * core::npack_indexing_v<Max - i, valueless_bias<NeverValueless>(Ns)...>
            );
        } else {
            return ofs * factor;
        }
    }

    static constexpr auto strides = []<std::size_t... Is>(std::index_sequence<Is...>) static consteval {
        return std::index_sequence<calc_single_stride<Is>(std::make_index_sequence<sizeof...(Ns)>{})...>{};
    }(std::make_index_sequence<sizeof...(Ns)>{});
};

// --------------------------------------------------

template<class R, class V, class n>
struct visit_impl;

template<class R, class... V, std::size_t... n>
struct visit_impl<
    R,
    core::type_list<V...>,
    std::index_sequence<n...>
>
{
    static_assert(sizeof...(V) == sizeof...(n));

    template<class Visitor, class... Variants>
    static constexpr R apply(Visitor&& vis, Variants&&... vars)  // NOLINT(cppcoreguidelines-missing-std-forward)
    {
        using VisitTable = visit_table<
            R,
            Visitor,
            core::type_list<forward_storage_t<as_variant_t<Variants>>...>, // Storage...
            core::seq_cartesian_product< // OverloadSeq
                std::index_sequence,
                std::make_index_sequence<
                    valueless_bias<std::remove_cvref_t<as_variant_t<Variants>>::never_valueless>(n)
                >...
            >
        >;

        std::size_t const flat_i = flat_index<
            std::index_sequence<n...>,
            std::integer_sequence<bool, std::remove_cvref_t<as_variant_t<Variants>>::never_valueless...>
        >::get(vars.index()...);

        constexpr auto const& table = VisitTable::table;
        auto const& f = table[flat_i];
        return std::invoke_r<R>(f, std::forward<Visitor>(vis), forward_storage<as_variant_t<Variants>>(vars)...);
    }
};

} // yk::detail

namespace yk {

template<
    class Visitor,
    class... Variants,
    // https://eel.is/c++draft/variant.visit#2
    class = std::void_t<detail::as_variant_t<Variants>...>
>
detail::visit_result_t<Visitor, detail::as_variant_t<Variants>...>
visit(Visitor&& vis, Variants&&... vars)
{
    using T0R = detail::visit_result_t<Visitor, detail::as_variant_t<Variants>...>;
    using Check = detail::visit_check<T0R, Visitor, detail::as_variant_t<Variants>...>;
    static_assert(
        Check::accepts_all_alternatives,
        "The spec mandates that the Visitor accept all combinations of alternative types "
        "(https://eel.is/c++draft/variant.visit#5)."
    );
    static_assert(
        Check::same_return_type,
        "The spec mandates that the Visitor return the same type and value category "
        "for all combinations of alternative types (https://eel.is/c++draft/variant.visit#5)."
    );
    return detail::visit_impl<
        T0R,
        core::type_list<detail::as_variant_t<Variants>...>,
        std::index_sequence<variant_size_v<std::remove_reference_t<detail::as_variant_t<Variants>>>...>
    >::apply(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template<
    class R,
    class Visitor,
    class... Variants,
    // https://eel.is/c++draft/variant.visit#2
    class = std::void_t<detail::as_variant_t<Variants>...>
>
R visit(Visitor&& vis, Variants&&... vars)
{
    using Check = detail::visit_R_check<R, Visitor, detail::as_variant_t<Variants>...>;
    static_assert(
        Check::accepts_all_alternatives,
        "The spec mandates that the Visitor accept all combinations of alternative types "
        "(https://eel.is/c++draft/variant.visit#5)."
    );
    static_assert(
        Check::return_type_convertible_to_R,
        "The spec mandates that each return type of the Visitor be implicitly convertible to `R` "
        "(https://eel.is/c++draft/variant.visit#6)."
    );

    return detail::visit_impl<
        R,
        core::type_list<detail::as_variant_t<Variants>...>,
        std::index_sequence<variant_size_v<std::remove_reference_t<detail::as_variant_t<Variants>>>...>
    >::apply(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

static_assert(std::is_convertible_v<void, void>);


} // yk

#endif
