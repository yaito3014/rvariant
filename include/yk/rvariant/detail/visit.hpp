#ifndef YK_RVARIANT_DETAIL_VISIT_HPP
#define YK_RVARIANT_DETAIL_VISIT_HPP

// Copyright 2025 Yaito Kakeyama
// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/rvariant/detail/rvariant_fwd.hpp>
#include <yk/rvariant/detail/variant_storage.hpp>
#include <yk/rvariant/variant_helper.hpp>

#include <yk/core/seq.hpp>
#include <yk/core/type_traits.hpp>

#include <variant> // std::bad_variant_access
#include <utility>
#include <functional>
#include <type_traits>

#include <cstddef>


namespace yk {

namespace detail {

[[noreturn]] inline void throw_bad_variant_access()
{
    throw std::bad_variant_access{};
}

#define YK_VISIT_CASES_0(def, ofs) \
    def(ofs); \
    def((ofs) + 1); \
    def((ofs) + 2); \
    def((ofs) + 3)

#define YK_VISIT_CASES_1(def, ofs) \
    YK_VISIT_CASES_0(def, ofs); \
    YK_VISIT_CASES_0(def, (ofs) + 4); \
    YK_VISIT_CASES_0(def, (ofs) + 8); \
    YK_VISIT_CASES_0(def, (ofs) + 12)

#define YK_VISIT_CASES_2(def, ofs) \
    YK_VISIT_CASES_1(def, ofs); \
    YK_VISIT_CASES_1(def, (ofs) + 16); \
    YK_VISIT_CASES_1(def, (ofs) + 32); \
    YK_VISIT_CASES_1(def, (ofs) + 48)

#define YK_VISIT_CASES_3(def, ofs) \
    YK_VISIT_CASES_2(def, ofs); \
    YK_VISIT_CASES_2(def, (ofs) + 64); \
    YK_VISIT_CASES_2(def, (ofs) + 128); \
    YK_VISIT_CASES_2(def, (ofs) + 192)

template<std::size_t OverloadSeqSize>
constexpr int visit_strategy =
    OverloadSeqSize <= 4 ? 0 :
    OverloadSeqSize <= 16 ? 1 :
    OverloadSeqSize <= 64 ? 2 :
    OverloadSeqSize <= 256 ? 3 : -1;


template<class Visitor, class Storage>
using raw_visit_result_t = decltype(std::declval<Visitor>()(
    std::declval<std::in_place_index_t<0>>(), std::declval<raw_get_t<0, Storage>>()
));

template<std::size_t I, class Visitor, class Storage>
struct raw_visit_noexcept
{
    template<class Storage_>
    struct lazy_invoke
    {
        static constexpr std::size_t RealI = detail::valueless_unbias<Storage_>(I);
        using type = std::is_nothrow_invocable<
            Visitor,
            std::in_place_index_t<RealI>,
            raw_get_t<RealI, Storage>
        >;
    };

    static constexpr bool value = std::conditional_t<
        !std::remove_cvref_t<Storage>::never_valueless && I == 0,
        std::type_identity<std::is_nothrow_invocable<
            Visitor,
            std::in_place_index_t<std::variant_npos>,
            decltype(std::forward_like<Storage>(std::declval<valueless_t>()))
        >>,
        lazy_invoke<Storage>
    >::type::value;
};


template<class Visitor, class Storage, class Is = std::make_index_sequence<detail::valueless_bias<Storage>(std::remove_cvref_t<Storage>::size)>>
constexpr bool raw_visit_noexcept_all = false;

template<class Visitor, class Storage, std::size_t... Is>
constexpr bool raw_visit_noexcept_all<Visitor, Storage, std::index_sequence<Is...>> = std::conjunction_v<
    raw_visit_noexcept<Is, Visitor, Storage>...
>;


template<std::size_t I, class Visitor, class Storage>
constexpr raw_visit_result_t<Visitor, Storage>
do_raw_visit(Visitor&& vis, Storage&& storage)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
    noexcept(raw_visit_noexcept<I, Visitor, Storage>::value)
{
    if constexpr (!std::remove_cvref_t<Storage>::never_valueless && I == 0) {
        return std::invoke(std::forward<Visitor>(vis), std::in_place_index<std::variant_npos>, std::forward<Storage>(storage));

    } else {
        constexpr std::size_t RealI = valueless_unbias<Storage>(I);
        return std::invoke(std::forward<Visitor>(vis), std::in_place_index<RealI>, raw_get<RealI>(std::forward<Storage>(storage)));
    }
}


template<class Visitor, class Storage>
using raw_visit_function_ptr = raw_visit_result_t<Visitor, Storage>(*) (Visitor&&, Storage&&)
    noexcept(raw_visit_noexcept_all<Visitor, Storage>);

template<class Visitor, class Storage, class Seq = std::make_index_sequence<detail::valueless_bias<Storage>(std::remove_cvref_t<Storage>::size)>>
struct raw_visit_table;

template<class Visitor, class Storage, std::size_t... Is>
struct raw_visit_table<Visitor, Storage, std::index_sequence<Is...>>
{
    static constexpr raw_visit_function_ptr<Visitor, Storage> table[] = {
        &do_raw_visit<Is, Visitor, Storage>...
    };
};

// --------------------------------------------

// When `raw_visit` is well-inlined,
// GCC yields false-positive "-Wmaybe-uninitialized" on
// the inactive `case` branch regardless of the actual
// value of `(n)`.
#ifdef NDEBUG // Release build
#if defined(__GNUC__) && !defined(__clang__)
# define YK_RVARIANT_DISABLE_UNINITIALIZED_WARNING_BEGIN \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")

# define YK_RVARIANT_DISABLE_UNINITIALIZED_WARNING_END \
    _Pragma("GCC diagnostic pop")

#else // non-GCC
# define YK_RVARIANT_DISABLE_UNINITIALIZED_WARNING_BEGIN
# define YK_RVARIANT_DISABLE_UNINITIALIZED_WARNING_END
#endif

#else // Debug build
# define YK_RVARIANT_DISABLE_UNINITIALIZED_WARNING_BEGIN
# define YK_RVARIANT_DISABLE_UNINITIALIZED_WARNING_END
#endif

template<bool NeverValueless, int Strategy>
struct raw_visit_dispatch;

template<bool NeverValueless>
struct raw_visit_dispatch<NeverValueless, -1>
{
    template<std::size_t N, class Visitor, class Storage>
    [[nodiscard]] YK_FORCEINLINE static constexpr raw_visit_result_t<Visitor, Storage>
    apply(std::size_t const i, [[maybe_unused]] Visitor&& vis, [[maybe_unused]] Storage&& storage)
        noexcept(raw_visit_noexcept_all<Visitor, Storage>)
    {
        constexpr auto const& table = raw_visit_table<Visitor, Storage>::table;
        auto const& f = table[i];
        return std::invoke(f, std::forward<Visitor>(vis), std::forward<Storage>(storage));
    }
};

#define YK_RAW_VISIT_NEVER_VALUELESS_CASE(n) \
    case (n): \
        if constexpr ((n) < N) { \
            return static_cast<Visitor&&>(vis)(std::in_place_index<(n)>, detail::raw_get<(n)>(static_cast<Storage&&>(storage))); \
        } else std::unreachable(); [[fallthrough]]

#define YK_RAW_VISIT_MAYBE_VALUELESS_CASE(n) \
    case (n) + 1: \
        if constexpr ((n) < N - 1) { \
            return static_cast<Visitor&&>(vis)(std::in_place_index<(n)>, detail::raw_get<(n)>(static_cast<Storage&&>(storage))); \
        } else std::unreachable(); [[fallthrough]]

#define YK_RAW_VISIT_DISPATCH_DEF(strategy) \
    template<> \
    struct raw_visit_dispatch<true, (strategy)> \
    { \
        template<std::size_t N, class Visitor, class Storage> \
        [[nodiscard]] YK_FORCEINLINE static constexpr detail::raw_visit_result_t<Visitor, Storage> \
        apply(std::size_t const i, [[maybe_unused]] Visitor&& vis, [[maybe_unused]] Storage&& storage) \
            noexcept(detail::raw_visit_noexcept_all<Visitor, Storage>) \
        { \
            static_assert(std::remove_cvref_t<Storage>::never_valueless); \
            static_assert((1uz << ((strategy) * 2uz)) <= N && N <= (1uz << (((strategy) + 1) * 2uz))); \
            switch (i) { \
            YK_VISIT_CASES_ ## strategy (YK_RAW_VISIT_NEVER_VALUELESS_CASE, 0); \
            default: std::unreachable(); \
            } \
        } \
    }; \
    template<> \
    struct raw_visit_dispatch<false, (strategy)> \
    { \
        template<std::size_t N, class Visitor, class Storage> \
        [[nodiscard]] YK_FORCEINLINE static constexpr detail::raw_visit_result_t<Visitor, Storage> \
        apply(std::size_t const i, [[maybe_unused]] Visitor&& vis, [[maybe_unused]] Storage&& storage) \
            noexcept(detail::raw_visit_noexcept_all<Visitor, Storage>) \
        { \
            static_assert(!std::remove_cvref_t<Storage>::never_valueless); \
            static_assert((1uz << ((strategy) * 2uz)) <= N && N <= (1uz << (((strategy) + 1) * 2uz))); \
            switch (i) { \
            case 0: return static_cast<Visitor&&>(vis)(std::in_place_index<std::variant_npos>, static_cast<Storage&&>(storage)); \
            YK_VISIT_CASES_ ## strategy (YK_RAW_VISIT_MAYBE_VALUELESS_CASE, 0); \
            default: std::unreachable(); \
            } \
        } \
    }

YK_RAW_VISIT_DISPATCH_DEF(0);
YK_RAW_VISIT_DISPATCH_DEF(1);
YK_RAW_VISIT_DISPATCH_DEF(2);
YK_RAW_VISIT_DISPATCH_DEF(3);

#undef YK_RAW_VISIT_NEVER_VALUELESS_CASE
#undef YK_RAW_VISIT_MAYBE_VALUELESS_CASE
#undef YK_RAW_VISIT_DISPATCH_DEF


template<class Variant, class Visitor>
YK_FORCEINLINE constexpr raw_visit_result_t<Visitor, forward_storage_t<Variant>>
raw_visit(Variant&& v, Visitor&& vis)  // NOLINT(cppcoreguidelines-missing-std-forward)
    noexcept(raw_visit_noexcept_all<Visitor, forward_storage_t<Variant>>)
{
    constexpr std::size_t N = detail::valueless_bias<Variant>(yk::variant_size_v<std::remove_reference_t<Variant>>);
    return raw_visit_dispatch<std::remove_cvref_t<Variant>::never_valueless, visit_strategy<N>>::template apply<N>(
        detail::valueless_bias<Variant>(v.index_),
        std::forward<Visitor>(vis),
        detail::forward_storage<Variant>(v)
    );
}

template<class Variant, class Visitor>
YK_FORCEINLINE constexpr raw_visit_result_t<Visitor, forward_storage_t<Variant>>
raw_visit_i(std::size_t const biased_i, Variant&& v, Visitor&& vis)  // NOLINT(cppcoreguidelines-missing-std-forward)
    noexcept(raw_visit_noexcept_all<Visitor, forward_storage_t<Variant>>)
{
    constexpr std::size_t N = detail::valueless_bias<Variant>(yk::variant_size_v<std::remove_reference_t<Variant>>);
    return raw_visit_dispatch<std::remove_cvref_t<Variant>::never_valueless, visit_strategy<N>>::template apply<N>(
        biased_i,
        std::forward<Visitor>(vis),
        detail::forward_storage<Variant>(v)
    );
}


// --------------------------------------------------

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
    unwrap_recursive(detail::raw_get<0>(forward_storage<Variants>(std::declval<Variants>())))...
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
    : std::conjunction<visit_check_impl<T0R, Visitor, core::type_list<Args..., unwrap_recursive_t<Ts>&>, Rest...>...> {};
template<class T0R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_check_impl<T0R, Visitor, core::type_list<Args...>, rvariant<Ts...> const&, Rest...>
    : std::conjunction<visit_check_impl<T0R, Visitor, core::type_list<Args..., unwrap_recursive_t<Ts> const&>, Rest...>...> {};
template<class T0R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_check_impl<T0R, Visitor, core::type_list<Args...>, rvariant<Ts...>&&, Rest...>
    : std::conjunction<visit_check_impl<T0R, Visitor, core::type_list<Args..., unwrap_recursive_t<Ts>>, Rest...>...> {};
template<class T0R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_check_impl<T0R, Visitor, core::type_list<Args...>, rvariant<Ts...> const&&, Rest...>
    : std::conjunction<visit_check_impl<T0R, Visitor, core::type_list<Args..., unwrap_recursive_t<Ts> const>, Rest...>...> {};

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
    : std::conjunction<visit_R_check_impl<R, Visitor, core::type_list<Args..., unwrap_recursive_t<Ts>&>, Rest...>...> {};
template<class R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_R_check_impl<R, Visitor, core::type_list<Args...>, rvariant<Ts...> const&, Rest...>
    : std::conjunction<visit_R_check_impl<R, Visitor, core::type_list<Args..., unwrap_recursive_t<Ts> const&>, Rest...>...> {};
template<class R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_R_check_impl<R, Visitor, core::type_list<Args...>, rvariant<Ts...>&&, Rest...>
    : std::conjunction<visit_R_check_impl<R, Visitor, core::type_list<Args..., unwrap_recursive_t<Ts>>, Rest...>...> {};
template<class R, class Visitor, class... Args, class... Ts, class... Rest>
struct visit_R_check_impl<R, Visitor, core::type_list<Args...>, rvariant<Ts...> const&&, Rest...>
    : std::conjunction<visit_R_check_impl<R, Visitor, core::type_list<Args..., unwrap_recursive_t<Ts> const>, Rest...>...> {};

template<class R, class Visitor, class... Variants>
using visit_R_check = visit_R_check_impl<R, Visitor, core::type_list<>, Variants...>;


// --------------------------------------------------

template<class R, class OverloadSeq, class Visitor, class... Storage>
struct multi_visit_noexcept;

template<class R, std::size_t... Is, class Visitor, class... Storage>
struct multi_visit_noexcept<R, std::index_sequence<Is...>, Visitor, Storage...>
{
private:
    template<class... Storage_>
    struct lazy_invoke
    {
        using type = std::is_nothrow_invocable_r<
            R,
            Visitor,
            unwrap_recursive_t<
                detail::raw_get_t<detail::valueless_unbias<Storage_>(Is), Storage_>
            >...
        >;
    };

public:
    static constexpr bool value = std::conditional_t<
        std::disjunction_v<
            std::conjunction<
                std::negation<storage_never_valueless<Storage>>,
                std::bool_constant<Is == 0>
            >...
        >,
        std::type_identity<std::bool_constant<false>>, // throw std::bad_variant_access{};
        lazy_invoke<Storage...>
    >::type::value;
};

template<class R, class... OverloadSeq, class Visitor, class... Storage>
struct multi_visit_noexcept<R, core::type_list<OverloadSeq...>, Visitor, Storage...>
    : std::conjunction<multi_visit_noexcept<R, OverloadSeq, Visitor, Storage...>...>
{};


template<class OverloadSeq>
struct multi_visitor;

template<std::size_t... Is>
struct multi_visitor<std::index_sequence<Is...>>
{
    template<class R, class Visitor, class... Storage>
    static constexpr R apply([[maybe_unused]] Visitor&& vis, [[maybe_unused]] Storage&&... storage)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
        YK_RVARIANT_VISIT_NOEXCEPT(multi_visit_noexcept<R, std::index_sequence<Is...>, Visitor, Storage...>::value)
    {
        if constexpr (((!std::remove_cvref_t<Storage>::never_valueless && Is == 0) || ...)) {
            detail::throw_bad_variant_access();

        } else {
#define YK_MULTI_VISITOR_INVOKE \
            std::invoke_r<R>( \
                std::forward<Visitor>(vis), \
                unwrap_recursive( \
                    raw_get<valueless_unbias<Storage>(Is)>(std::forward<Storage>(storage)) \
                )... \
            )
#if YK_CI
            static_assert(
                noexcept(YK_MULTI_VISITOR_INVOKE)
                == multi_visit_noexcept<R, std::index_sequence<Is...>, Visitor, Storage...>::value
            );
#endif
            return YK_MULTI_VISITOR_INVOKE;
#undef YK_MULTI_VISITOR_INVOKE
        }
    }
};

template<class R, class OverloadSeq, class Visitor, class... Storage>
struct visit_table;

template<class R, class... OverloadSeq, class Visitor, class... Storage>
struct visit_table<
    R,
    core::type_list<OverloadSeq...>,
    Visitor,
    Storage...
>
{
    using function_type = R(*)(Visitor&&, Storage&&...);

    static constexpr function_type table[] = {
        &multi_visitor<OverloadSeq>::template apply<R, Visitor, Storage...>...
    };
};

template<int Strategy>
struct visit_dispatch;

template<>
struct visit_dispatch<-1>
{
    template<class R, class OverloadSeq, class Visitor, class... Storage>
    [[nodiscard]] YK_FORCEINLINE static constexpr R apply(std::size_t const flat_i, [[maybe_unused]] Visitor&& vis, [[maybe_unused]] Storage&&... storage)
        YK_RVARIANT_VISIT_NOEXCEPT(multi_visit_noexcept<R, OverloadSeq, Visitor, Storage...>::value)
    {
        constexpr auto const& table = visit_table<R, OverloadSeq, Visitor, Storage...>::table;
        auto const& f = table[flat_i];
        return std::invoke_r<R>(f, std::forward<Visitor>(vis), std::forward<Storage>(storage)...);
    }
};

#define YK_VISIT_CASE(n) \
    case (n): \
        if constexpr ((n) < OverloadSeq::size) { \
            return multi_visitor<core::at_c_t<(n), OverloadSeq>>::template apply<R, Visitor, Storage...>( \
                static_cast<Visitor&&>(vis), static_cast<Storage&&>(storage)... \
            ); \
        } else std::unreachable(); [[fallthrough]]

#define YK_VISIT_DISPATCH_DEF(strategy) \
    template<> \
    struct visit_dispatch<(strategy)> \
    { \
        template<class R, class OverloadSeq, class Visitor, class... Storage> \
        [[nodiscard]] static constexpr R apply(std::size_t const flat_i, [[maybe_unused]] Visitor&& vis, [[maybe_unused]] Storage&&... storage) \
            YK_RVARIANT_VISIT_NOEXCEPT(multi_visit_noexcept<R, OverloadSeq, Visitor, Storage...>::value) \
        { \
            static_assert((1uz << ((strategy) * 2uz)) <= OverloadSeq::size && OverloadSeq::size <= (1uz << (((strategy) + 1) * 2uz))); \
            switch (flat_i) { \
            YK_VISIT_CASES_ ## strategy (YK_VISIT_CASE, 0); \
            default: std::unreachable(); \
            } \
        } \
    }

YK_VISIT_DISPATCH_DEF(0);
YK_VISIT_DISPATCH_DEF(1);
YK_VISIT_DISPATCH_DEF(2);
YK_VISIT_DISPATCH_DEF(3);

#undef YK_VISIT_DISPATCH_DEF
#undef YK_VISIT_CASE

#undef YK_VISIT_CASES_0
#undef YK_VISIT_CASES_1
#undef YK_VISIT_CASES_2
#undef YK_VISIT_CASES_3


template<class Ns, bool... NeverValueless>
struct flat_index;

template<std::size_t... Ns, bool... NeverValueless>
struct flat_index<std::index_sequence<Ns...>, NeverValueless...>
{
    template<class... RuntimeIndex>
    [[nodiscard]] YK_FORCEINLINE static constexpr std::size_t
    get(RuntimeIndex... index) noexcept
    {
        static_assert(sizeof...(RuntimeIndex) == sizeof...(Ns));
        assert(((detail::valueless_bias<NeverValueless>(index) < detail::valueless_bias<NeverValueless>(Ns)) && ...));
        return flat_index::get_impl(flat_index::strides, index...);
    }

private:
    template<std::size_t... Stride, class... RuntimeIndex>
    [[nodiscard]] YK_FORCEINLINE static constexpr std::size_t
    get_impl(std::index_sequence<Stride...>, RuntimeIndex... index) noexcept
    {
        return ((Stride * detail::valueless_bias<NeverValueless>(index)) + ...);
    }

    static constexpr std::size_t Ns_i_max = sizeof...(Ns) - 1;

    template<std::size_t Ns_i, std::size_t i, std::size_t... i_rest, std::size_t... biased_Ns>
    [[nodiscard]] static consteval std::size_t
    calc_single_stride(std::index_sequence<i, i_rest...>, std::index_sequence<biased_Ns...>, std::size_t stride = 1) noexcept
    {
        if constexpr (sizeof...(i_rest) == 0 || (Ns_i_max - i) == Ns_i) {
            return stride;
        } else {
            return flat_index::calc_single_stride<Ns_i>(
                std::index_sequence<i_rest...>{},
                std::index_sequence<biased_Ns...>{},
                stride * core::npack_indexing_v<Ns_i_max - i, biased_Ns...>
            );
        }
    }

    static constexpr auto strides = []<std::size_t... Ns_i>(std::index_sequence<Ns_i...>) static consteval {
        return std::index_sequence<flat_index::calc_single_stride<Ns_i>(
            std::make_index_sequence<sizeof...(Ns)>{},
            std::index_sequence<detail::valueless_bias<NeverValueless>(Ns)...>{}
        )...>{};
    }(std::make_index_sequence<sizeof...(Ns)>{});
};


template<class... Variants>
using make_OverloadSeq = core::seq_cartesian_product<
    std::index_sequence,
    std::make_index_sequence<
        detail::valueless_bias<detail::as_variant_t<Variants>>(
            yk::variant_size_v<std::remove_reference_t<detail::as_variant_t<Variants>>> // aka `n`
        )
    >...
>;

template<class R, class V, std::size_t... n>
struct visit_impl;

template<class R, class... V, std::size_t... n>
struct visit_impl<
    R,
    core::type_list<V...>,
    n...
>
{
    template<class Visitor, class... Variants, class OverloadSeq = make_OverloadSeq<Variants...>>
    static constexpr R apply(Visitor&& vis, Variants&&... vars)  // NOLINT(cppcoreguidelines-missing-std-forward)
        YK_RVARIANT_VISIT_NOEXCEPT(multi_visit_noexcept<R, OverloadSeq, Visitor, forward_storage_t<as_variant_t<Variants>>...>::value)
    {
        std::size_t const flat_i = flat_index<
            std::index_sequence<n...>,
            std::remove_cvref_t<as_variant_t<Variants>>::never_valueless...
        >::get(vars.index_...);

        return visit_dispatch<visit_strategy<OverloadSeq::size>>::template apply<R, OverloadSeq>(
            flat_i, std::forward<Visitor>(vis), forward_storage<as_variant_t<Variants>>(vars)...
        );
    }
};

} // detail


template<
    class Visitor,
    class... Variants,
    // https://eel.is/c++draft/variant.visit#2
    class = std::void_t<detail::as_variant_t<Variants>...>
>
detail::visit_result_t<Visitor, detail::as_variant_t<Variants>...>
YK_FORCEINLINE constexpr visit(Visitor&& vis, Variants&&... vars)
    YK_RVARIANT_VISIT_NOEXCEPT(detail::multi_visit_noexcept<
        detail::visit_result_t<Visitor, detail::as_variant_t<Variants>...>,
        detail::make_OverloadSeq<Variants...>,
        Visitor,
        detail::forward_storage_t<detail::as_variant_t<Variants>>...
    >::value)
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
        variant_size_v<std::remove_reference_t<detail::as_variant_t<Variants>>>...
    >::apply(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template<
    class R,
    class Visitor,
    class... Variants,
    // https://eel.is/c++draft/variant.visit#2
    class = std::void_t<detail::as_variant_t<Variants>...>
>
YK_FORCEINLINE constexpr R visit(Visitor&& vis, Variants&&... vars)
    YK_RVARIANT_VISIT_NOEXCEPT(detail::multi_visit_noexcept<
        R,
        detail::make_OverloadSeq<Variants...>,
        Visitor,
        detail::forward_storage_t<detail::as_variant_t<Variants>>...
    >::value)
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
        variant_size_v<std::remove_reference_t<detail::as_variant_t<Variants>>>...
    >::apply(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

} // yk

#endif
