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

namespace yk {

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

template<class Source, class... Ts, std::size_t... Is>
struct fun<Source, rvariant<Ts...>, std::index_sequence<Is...>> : fun_impl<Is, pack_indexing_t<Is, Ts...>, Source>... {
    using fun_impl<Is, pack_indexing_t<Is, Ts...>, Source>::operator()...;
};

template<class Source, class Variant>
using accepted_index = decltype(fun<Source, Variant>{}(std::declval<Source>()));

template<class Source, class Variant>
inline constexpr std::size_t accepted_index_v = accepted_index<Source, Variant>::value;

template<class Source, class Variant>
struct accepted_type;

template<class Source, class... Ts>
struct accepted_type<Source, rvariant<Ts...>> {
    using type = pack_indexing_t<accepted_index_v<Source, rvariant<Ts...>>, Ts...>;
};

template<class Source, class Variant>
using accepted_type_t = typename accepted_type<Source, Variant>::type;


template<class T, class... Ts>
struct has_recursive_wrapper_duplicate
    : std::false_type {};

template<class T, class Allocator, class... Ts>
struct has_recursive_wrapper_duplicate<recursive_wrapper<T, Allocator>, Ts...>
    : is_in<T, Ts...> {};


template<class Variant, class T>
struct exactly_once_index
{
    static_assert(exactly_once_v<T, typename Variant::unwrapped_types>);
    static constexpr std::size_t value = find_index_v<T, typename Variant::unwrapped_types>;
};

template<class Variant, class T>
inline constexpr std::size_t exactly_once_index_v = exactly_once_index<Variant, T>::value;

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
    constexpr rvariant() noexcept(std::is_nothrow_default_constructible_v<detail::pack_indexing_t<0, Ts...>>)
        requires std::is_default_constructible_v<detail::pack_indexing_t<0, Ts...>>
        : storage_{}
        , index_(0)
    {}

    constexpr rvariant(rvariant const&) requires detail::all_trivially_copy_constructible<Ts...> = default;
    constexpr rvariant(rvariant const&) requires (!detail::all_copy_constructible<Ts...>) = delete;

    constexpr rvariant(rvariant const& other) requires detail::all_copy_constructible<Ts...>
        : storage_(detail::valueless)
        , index_(other.index_)
    {
        detail::raw_visit(
            index_,
            [this]<std::size_t I, class T>(detail::alternative<I, T> const& alt) {
                if constexpr (I != std::variant_npos) {
                    std::construct_at(&storage_, std::in_place_index<I>, alt.value);
                }
            },
            other
        );
    }

    constexpr rvariant(rvariant&&) requires detail::all_trivially_move_constructible<Ts...> = default;

    constexpr rvariant(rvariant&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
        requires detail::all_move_constructible<Ts...>
        : storage_(detail::valueless)
        , index_(other.index_)
    {
        variant_npos_setter guard(this);
        detail::raw_visit(
            index_,
            [this]<std::size_t I, class T>(detail::alternative<I, T>&& alt) {
                if constexpr (I != std::variant_npos) {
                    std::construct_at(&storage_, std::in_place_index<I>, std::move(alt).value);
                }
            },
            std::move(other)
        );
        guard.mark_succeeded();
    }


    // Flexible copy constructor
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            subset_of<rvariant<Us...>, rvariant> &&
            detail::all_copy_constructible<Us...>
    constexpr rvariant(rvariant<Us...> const& other)
        : storage_(detail::valueless)
        , index_(detail::subset_reindex<rvariant<Us...>, rvariant>(other.index_))
    {
        detail::raw_visit(
            other.index_,
            [this]<std::size_t I, class T>(detail::alternative<I, T> const& alt) {
                if constexpr (I != std::variant_npos) {
                    std::construct_at(
                        &storage_,
                        std::in_place_index<detail::subset_reindex<rvariant<Us...>, rvariant>(I)>,
                        alt.value
                    );
                }
            },
            other
        );
    }

    template<class... Us>
        requires (!(
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            subset_of<rvariant<Us...>, rvariant> &&
            detail::all_copy_constructible<Us...>
        ))
    constexpr rvariant(rvariant<Us...> const&) = delete;


    // Flexible move constructor
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            subset_of<rvariant<Us...>, rvariant> &&
            detail::all_move_constructible<Us...>
    constexpr rvariant(rvariant<Us...>&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Us>...>)
        : storage_(detail::valueless)
        , index_(detail::subset_reindex<rvariant<Us...>, rvariant>(other.index_))
    {
        variant_npos_setter guard(this);
        detail::raw_visit(
            other.index_,
            [this]<std::size_t I, class T>(detail::alternative<I, T>&& alt) {
                if constexpr (I != std::variant_npos) {
                    std::construct_at(
                        &storage_,
                        std::in_place_index<detail::subset_reindex<rvariant<Us...>, rvariant>(I)>,
                        std::move(alt).value
                    );
                }
            },
            std::move(other)
        );
        guard.mark_succeeded();
    }

    template<class... Us>
        requires (!(
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            subset_of<rvariant<Us...>, rvariant> &&
            detail::all_move_constructible<Us...>
        ))
    constexpr rvariant(rvariant<Us...>&&) = delete;


    // Generic constructor
    template<class T>
        requires requires {
            requires sizeof...(Ts) > 0;
            requires !std::is_same_v<std::remove_cvref_t<T>, rvariant>;
            requires !detail::is_ttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_type_t>;
            requires !detail::is_nttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_index_t>;
            requires requires(T source) { detail::fun<T, rvariant>{}(std::forward<T>(source)); };
            requires std::is_constructible_v<detail::accepted_type_t<T, rvariant>, T>;
        }
    constexpr rvariant(T&& x)
        noexcept(std::is_nothrow_constructible_v<detail::accepted_type_t<T, rvariant>, T>)
        : rvariant(std::in_place_index<detail::accepted_index_v<T, rvariant>>, std::forward<T>(x))
    {}

    // in_place_type<T>, args...
    template<class T, class... Args>
        requires
            detail::exactly_once_v<T, unwrapped_types> &&
            std::is_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, Args...>
    constexpr explicit rvariant(std::in_place_type_t<T>, Args&&... args)
        : rvariant(std::in_place_index<detail::select_maybe_wrapped_index<T, Ts...>>, std::forward<Args>(args)...)
    {
        static_assert(!detail::is_ttp_specialization_of_v<T, recursive_wrapper>);
    }

    // in_place_type<T>, il, args...
    template<class T, class U, class... Args>
        requires
            detail::exactly_once_v<T, unwrapped_types> &&
            std::is_constructible_v<detail::select_maybe_wrapped_t<T, Ts...>, std::initializer_list<U>&, Args...>
    constexpr explicit rvariant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
        : rvariant(std::in_place_index<detail::select_maybe_wrapped_index<T, Ts...>>, il, std::forward<Args>(args)...)
    {
        static_assert(!detail::is_ttp_specialization_of_v<T, recursive_wrapper>);
    }

    // in_place_index<I>, args...
    template<std::size_t I, class... Args>
        requires
            (I < sizeof...(Ts)) &&
            std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, Args...>
    constexpr explicit rvariant(std::in_place_index_t<I>, Args&&... args)
        : storage_(std::in_place_index<I>, std::forward<Args>(args)...)
        , index_(I)
    {}

    // in_place_index<I>, il, args...
    template<std::size_t I, class U, class... Args>
        requires
            (I < sizeof...(Ts)) &&
            std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
    constexpr explicit rvariant(std::in_place_index_t<I>, std::initializer_list<U> il, Args&&... args)
        : storage_(std::in_place_index<I>, il, std::forward<Args>(args)...)
        , index_(I)
    {}

    constexpr ~rvariant() = default;

    constexpr ~rvariant()
        requires (!std::conjunction_v<std::is_trivially_destructible<Ts>...>)
    {
        reset();
    }

    constexpr rvariant& operator=(rvariant const&)
        requires detail::variant_trivially_copy_assignable<Ts...>
    = default;

    constexpr rvariant& operator=(rvariant const& other)
        requires detail::variant_copy_assignable<Ts...>
    {
        detail::raw_visit(
            other.index_,
            [this]<class Alt>(Alt const& alt) {
                constexpr std::size_t I = std::remove_cvref_t<Alt>::index;
                using T = typename std::remove_cvref_t<Alt>::type;
                if constexpr (I == std::variant_npos) {
                    reset();
                } else {
                    if (index_ == I) {
                        // directly assign
                        detail::raw_get<I>(*this).value = alt.value;
                    } else {
                        if constexpr (std::is_nothrow_copy_constructible_v<T> || !std::is_nothrow_move_constructible_v<T>) {
                            emplace<I>(alt.value);
                        } else {
                            emplace<I>(T(alt.value));
                        }
                        index_ = I;
                    }
                }
            },
            other
        );
        return *this;
    }

    constexpr rvariant& operator=(rvariant const&)
        requires (!detail::variant_copy_assignable<Ts...>)
    = delete;

    // --------------------------------------------

    constexpr rvariant& operator=(rvariant&&)
        requires detail::variant_trivially_move_assignable<Ts...>
    = default;

    constexpr rvariant& operator=(rvariant&& other)
        noexcept(std::conjunction_v<
            std::conjunction<std::is_nothrow_move_constructible<Ts>, std::is_nothrow_move_assignable<Ts>>...
        >)
        requires detail::variant_move_assignable<Ts...>
    {
        detail::raw_visit(
            other.index_,
            [this]<class Alt>(Alt&& alt) {
                constexpr std::size_t I = std::remove_cvref_t<Alt>::index;
                if constexpr (I == std::variant_npos) {
                    reset();
                } else {
                    if (index_ == I) {
                        detail::raw_get<I>(*this).value = std::move(alt).value;
                    } else {
                        emplace<I>(std::move(alt).value);
                    }
                }
            },
            other
        );
        return *this;
    }

    // --------------------------------------

    // Generic assignment operator
    template<class T>
        requires requires {
            requires (!std::is_same_v<std::remove_cvref_t<T>, rvariant>);
            requires requires(T source) { detail::fun<T, rvariant>{}(std::forward<T>(source)); };
            requires std::is_assignable_v<detail::accepted_type_t<T, rvariant>&, T> && std::is_constructible_v<detail::accepted_type_t<T, rvariant>, T>;
        }
    constexpr rvariant& operator=(T&& arg)
        noexcept(
            std::is_nothrow_assignable_v<detail::accepted_type_t<T, rvariant>&, T> &&
            std::is_nothrow_constructible_v<detail::accepted_type_t<T, rvariant>, T>
        )
    {
        detail::raw_visit(
            index_,
            [this, &arg]<class Alt>(Alt&& alt) {
                constexpr std::size_t I = std::remove_cvref_t<Alt>::index;
                constexpr std::size_t J = detail::accepted_index_v<T, rvariant>;
                using Assignee = typename std::remove_cvref_t<Alt>::type;
                using Assigner = detail::accepted_type_t<T, rvariant>;
                if constexpr (I != std::variant_npos) {
                    if constexpr (std::is_same_v<Assignee, Assigner>) {
                        alt.value = std::forward<T>(arg);
                    } else {
                        if constexpr (std::is_nothrow_constructible_v<Assigner, T> || !std::is_nothrow_move_constructible_v<Assigner>) {
                            emplace<J>(std::forward<T>(arg));
                        } else {
                            emplace<J>(Assigner(std::forward<T>(arg)));
                        }
                    }
                }
            },
            *this
        );
        return *this;
    }

    // Flexible copy assignment operator
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            subset_of<rvariant<Us...>, rvariant> &&
            detail::variant_copy_assignable<Us...>
    constexpr rvariant& operator=(rvariant<Us...> const& other)
    {
        detail::raw_visit(
            other.index_,
            [&self = *this]<class Alt>(Alt const& alt) {
                constexpr std::size_t I = std::remove_cvref_t<Alt>::index;
                if constexpr (I == std::variant_npos) {
                    self.reset();

                } else {
                    using T = typename std::remove_cvref_t<Alt>::type;

                    bool const was_same_alternative = detail::raw_visit(
                        self.index_,

                        // directly assign if both alternative is same type and returns true; otherwise returns false
                        [&alt]<class ThisAlt>(ThisAlt&& thisAlt) {
                            constexpr std::size_t K = std::remove_cvref_t<ThisAlt>::index;
                            if constexpr (K == std::variant_npos) {
                                return false;

                            } else {
                                using U = std::remove_cvref_t<ThisAlt>::type;
                                if constexpr (std::is_same_v<T, U>) {
                                    thisAlt.value = alt.value;
                                    return true;

                                } else {
                                    return false;
                                }
                            }
                        },
                        self
                    );
                    if (was_same_alternative) return;

                    constexpr std::size_t J = detail::subset_reindex<rvariant<Us...>, rvariant>(I);
                    if constexpr (std::is_nothrow_copy_constructible_v<T> || !std::is_nothrow_move_constructible_v<T>) {
                        self.template emplace<J>(alt.value);

                    } else {
                        self.template emplace<J>(T(alt.value));
                    }
                }
            },
            other
        );
        return *this;
    }

    // Flexible move assignment operator
    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            subset_of<rvariant<Us...>, rvariant> &&
            detail::variant_move_assignable<Us...>
    constexpr rvariant& operator=(rvariant<Us...>&& other) noexcept(
        std::conjunction_v<
            std::conjunction<
                std::is_nothrow_move_constructible<Us>,
                std::is_nothrow_move_assignable<Us>
            >...
        >
    ) {
        detail::raw_visit(
            other.index_,
            [this]<class Alt>(Alt&& alt) {
                constexpr std::size_t I = std::remove_cvref_t<Alt>::index;
                if constexpr (I == std::variant_npos) {
                    reset();

                } else {
                    using T = typename std::remove_cvref_t<Alt>::type;

                    bool const was_same_alternative = detail::raw_visit(
                        index_,
                        // directly assign if both alternative is same type and returns true; otherwise returns false
                        [this, &alt]<class ThisAlt>(ThisAlt&& thisAlt) {
                            constexpr std::size_t K = std::remove_cvref_t<ThisAlt>::index;
                            if constexpr (K == std::variant_npos) {
                                return false;

                            } else {
                                using U = std::remove_cvref_t<ThisAlt>::type;
                                if constexpr (std::is_same_v<T, U>) {
                                    thisAlt.value = std::forward<Alt>(alt).value;
                                    return true;
                                } else {
                                    return false;
                                }
                            }
                        },
                        *this
                    );
                    if (was_same_alternative) return;

                    constexpr std::size_t J = detail::subset_reindex<rvariant<Us...>, rvariant>(I);
                    emplace<I>(std::forward<Alt>(alt).value);
                }
            },
            std::move(other)
        );
        return *this;
    }

    // -------------------------------------------

    template<class T, class... Args>
        requires std::conjunction_v<std::is_constructible<T, Args...>, detail::exactly_once<T, unwrapped_types>>
    constexpr T& emplace(Args&&... args) YK_LIFETIMEBOUND
    {
        static_assert(!detail::is_ttp_specialization_of_v<T, recursive_wrapper>);
        constexpr std::size_t I = detail::find_index_v<T, unwrapped_types>;
        return emplace<I>(std::forward<Args>(args)...);
    }

    template<class T, class U, class... Args>
        requires std::conjunction_v<std::is_constructible<T, std::initializer_list<U>&, Args...>, detail::exactly_once<T, unwrapped_types>>
    constexpr T& emplace(std::initializer_list<U> il, Args&&... args) YK_LIFETIMEBOUND
    {
        static_assert(!detail::is_ttp_specialization_of_v<T, recursive_wrapper>);
        constexpr std::size_t I = detail::find_index_v<T, unwrapped_types>;
        return emplace<I>(il, std::forward<Args>(args)...);
    }

    template<std::size_t I, class... Args>
        requires std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, Args...>
    constexpr variant_alternative_t<I, rvariant>&
        emplace(Args&&... args) YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        if (!valueless_by_exception()) {
            reset();
        }
        std::construct_at(&storage_, std::in_place_index<I>, std::forward<Args>(args)...);
        index_ = I;
        return detail::unwrap_recursive(detail::raw_get<I>(*this).value);
    }

    template<std::size_t I, class U, class... Args>
        requires std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
    constexpr variant_alternative_t<I, rvariant>&
        emplace(std::initializer_list<U> il, Args&&... args) YK_LIFETIMEBOUND
    {
        static_assert(I < sizeof...(Ts));
        if (!valueless_by_exception()) {
            reset();
        }
        std::construct_at(&storage_, std::in_place_index<I>, il, std::forward<Args>(args)...);
        index_ = I;
        return detail::unwrap_recursive(detail::raw_get<I>(*this).value);
    }

    constexpr void swap(rvariant& other)
        noexcept(std::conjunction_v<std::conjunction<
            std::is_nothrow_move_constructible<Ts>,
            std::is_nothrow_swappable<Ts>
        >...>)
    {
        static_assert(std::conjunction_v<std::is_move_constructible<Ts>...>);
        detail::raw_visit(
            index_,
            [&self = *this, &other]<class ThisAlt>(ThisAlt&& thisAlt) {
                detail::raw_visit(
                    other.index_,
                    [&self, &thisAlt, &other]<class OtherAlt>(OtherAlt&& otherAlt) {
                        constexpr std::size_t I = std::remove_cvref_t<ThisAlt>::index;
                        constexpr std::size_t J = std::remove_cvref_t<OtherAlt>::index;

                        if constexpr (I == J) {
                            if constexpr (I != std::variant_npos) {
                                using std::swap;
                                swap(detail::raw_get<I>(self).value, detail::raw_get<I>(other).value);
                            }

                        } else if constexpr (I == std::variant_npos) {
                            self.template emplace<J>(std::forward<OtherAlt>(otherAlt).value);
                            other.template reset<J>();

                        } else if constexpr (J == std::variant_npos) {
                            other.template emplace<I>(std::forward<ThisAlt>(thisAlt).value);
                            self.template reset<I>();

                        } else {
                            auto temporary = std::forward<ThisAlt>(thisAlt).value;
                            self.template reset<I>();
                            self.template emplace<J>(std::forward<OtherAlt>(otherAlt).value);
                            other.template reset<J>();
                            other.template emplace<I>(std::move(temporary));
                        }
                    },
                    other
                );
            },
            *this
        );
    }

    friend constexpr void swap(rvariant& v, rvariant& w)
        noexcept(noexcept(v.swap(w)))
        requires (std::conjunction_v<std::conjunction<
            std::is_move_constructible<Ts>,
            std::is_swappable<Ts>
        >...>)
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
            subset_of<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant<Us...> subset() const&
        noexcept(
            equivalent_to<rvariant<Us...>, rvariant> &&
            std::is_nothrow_copy_constructible_v<rvariant>
        )
    {
        return detail::raw_visit(index_, detail::subset_visitor<rvariant, Us...>{}, *this);
    }

    template<class... Us>
        requires
            (!std::is_same_v<rvariant<Us...>, rvariant>) &&
            subset_of<rvariant<Us...>, rvariant>
    [[nodiscard]] constexpr rvariant<Us...> subset() &&
        noexcept(
            equivalent_to<rvariant<Us...>, rvariant> &&
            std::is_nothrow_move_constructible_v<rvariant>
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
    class variant_npos_setter
    {
    public:
        constexpr variant_npos_setter(rvariant* par) noexcept : parent_(par) {}
        constexpr void mark_succeeded() noexcept { succeeded_ = true; }

        constexpr ~variant_npos_setter() noexcept
        {
            if (!succeeded_) parent_->index_ = std::variant_npos;
        }

    private:
        rvariant* parent_;
        bool succeeded_ = false;
    };
    friend class variant_npos_setter;

    constexpr rvariant(detail::valueless_t) noexcept
        : storage_(detail::valueless)
        , index_(std::variant_npos)
    {}

    constexpr void destroy() noexcept
    {
        detail::raw_visit(
            index_,
            []<class Alt>(Alt&& alt) {
                if constexpr (std::remove_cvref_t<Alt>::index != std::variant_npos) {
                    using T = typename std::remove_cvref_t<Alt>::type;
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

    constexpr void reset() noexcept
    {
        destroy();
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
    constexpr std::size_t I = detail::exactly_once_index_v<rvariant<Ts...>, T>;
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
    constexpr std::size_t I = detail::exactly_once_index_v<rvariant<Ts...>, T>;
    if (var.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(var).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T&&
get(rvariant<Ts...>&& var YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<rvariant<Ts...>, T>;
    if (var.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(std::move(var)).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T const&
get(rvariant<Ts...> const& var YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<rvariant<Ts...>, T>;
    if (var.index() != I) throw std::bad_variant_access{};
    return detail::unwrap_recursive(detail::raw_get<I>(var).value);
}

template<class T, class... Ts>
[[nodiscard]] constexpr T const&&
get(rvariant<Ts...> const&& var YK_LIFETIMEBOUND)
{
    constexpr std::size_t I = detail::exactly_once_index_v<rvariant<Ts...>, T>;
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
    constexpr std::size_t I = detail::exactly_once_index_v<rvariant<Ts...>, T>;
    return get_if<I>(var);
}

template<class T, class... Ts>
[[nodiscard]] constexpr std::add_pointer_t<T const>
get_if(rvariant<Ts...> const* var) noexcept
{
    constexpr std::size_t I = detail::exactly_once_index_v<rvariant<Ts...>, T>;
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
