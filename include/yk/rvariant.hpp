#ifndef YK_RVARIANT_HPP
#define YK_RVARIANT_HPP

#include "yk/detail/exactly_once.hpp"
#include "yk/detail/is_specialization_of.hpp"
#include "yk/detail/pack_indexing.hpp"

#include <initializer_list>
#include <type_traits>
#include <utility>
#include <variant>

#include <cstddef>

namespace yk {

template <class... Ts>
class rvariant;

template <class Variant>
struct variant_size;

template <class Variant>
inline constexpr std::size_t variant_size_v = variant_size<Variant>::value;

template <class Variant>
struct variant_size<const Variant> : variant_size<Variant> {};

template <class... Ts>
struct variant_size<rvariant<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <std::size_t I, class Variant>
struct variant_alternative;

template <std::size_t I, class Variant>
using variant_alternative_t = typename variant_alternative<I, Variant>::type;

template <std::size_t I, class Variant>
struct variant_alternative<I, const Variant> : std::add_const<variant_alternative_t<I, Variant>> {};

template <std::size_t I, class... Ts>
struct variant_alternative<I, rvariant<Ts...>> : detail::pack_indexing<I, Ts...> {
  static_assert(I < sizeof...(Ts));
};

inline constexpr std::size_t variant_npos = -1;

namespace detail {

template <class... Ts>
concept all_copy_constructible = std::conjunction_v<std::is_copy_constructible<Ts>...>;

template <class... Ts>
concept all_trivially_copy_constructible = all_copy_constructible<Ts...> && std::conjunction_v<std::is_trivially_copy_constructible<Ts>...>;

template <class... Ts>
concept all_move_constructible = std::conjunction_v<std::is_move_constructible<Ts>...>;

template <class... Ts>
concept all_trivially_move_constructible = all_move_constructible<Ts...> && std::conjunction_v<std::is_trivially_move_constructible<Ts>...>;

template <std::size_t I, class Dest, class Source>
struct fun_impl {
  std::integral_constant<std::size_t, I> operator()(Dest arg)
    requires requires(Source source) { (Dest[]){std::forward<Source>(source)}; };
};

template <class Source, class Variant, class Seq = std::make_index_sequence<variant_size_v<Variant>>>
struct fun;

template <class Source, class Variant, std::size_t... Is>
struct fun<Source, Variant, std::index_sequence<Is...>> : fun_impl<Is, variant_alternative_t<Is, Variant>, Source>... {
  using fun_impl<Is, variant_alternative_t<Is, Variant>, Source>::operator()...;
};

template <class Source, class Variant>
using accepted_index = decltype(fun<Source, Variant>{}(std::declval<Source>()));

template <class Source, class Variant>
inline constexpr std::size_t accepted_index_v = accepted_index<Source, Variant>::value;

struct valueless_t {};

inline constexpr valueless_t valueless{};

template <bool TriviallyDestructible, class... Ts>
union variadic_union;

template <bool TriviallyDestructible>
union variadic_union<TriviallyDestructible> {
  constexpr variadic_union(valueless_t) {}
};

template <bool TriviallyDestructible, class T, class... Ts>
union variadic_union<TriviallyDestructible, T, Ts...> {
  constexpr variadic_union() : first() {}

  constexpr variadic_union(valueless_t) : rest(valueless) {}

  template <class... Args>
  constexpr variadic_union(std::in_place_index_t<0>, Args&&... args) : first(std::forward<Args>(args)...) {}

  template <std::size_t I, class... Args>
  constexpr variadic_union(std::in_place_index_t<I>, Args&&... args) : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...) {}

  ~variadic_union() = default;

  constexpr ~variadic_union()
    requires(!TriviallyDestructible)
  {}

  T first;
  variadic_union<TriviallyDestructible, Ts...> rest;
};

template <std::size_t I>
struct get_alternative {
  template <class Union>
  constexpr auto&& operator()(Union&& u) noexcept {
    return get_alternative<I - 1>{}(std::forward<Union>(u).rest);
  }
};

template <>
struct get_alternative<0> {
  template <class Union>
  constexpr auto&& operator()(Union&& u) noexcept {
    return std::forward<Union>(u).first;
  }
};

template <std::size_t I, class Variant>
constexpr decltype(auto) get_impl(Variant&& var) noexcept {
  return get_alternative<I>{}(std::forward<Variant>(var).storage_);
}

}  // namespace detail

template <class... Ts>
class rvariant {
public:
  static_assert(detail::is_unique_v<Ts...>);

  constexpr rvariant() noexcept(std::is_nothrow_default_constructible_v<detail::pack_indexing_t<0, Ts...>>)
    requires std::is_default_constructible_v<detail::pack_indexing_t<0, Ts...>>
      : storage_{}, index_(0) {}

  constexpr rvariant(const rvariant&)
    requires detail::all_trivially_copy_constructible<Ts...>
  = default;

  constexpr rvariant(const rvariant&)
    requires(!detail::all_copy_constructible<Ts...>)
  = delete;

  constexpr rvariant(const rvariant& other)
    requires detail::all_copy_constructible<Ts...>
      : storage_(detail::valueless), index_(other.index_) {
    // TODO
  }

  constexpr rvariant(rvariant&&)
    requires detail::all_trivially_move_constructible<Ts...>
  = default;

  constexpr rvariant(rvariant&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
    requires detail::all_move_constructible<Ts...>
      : storage_(detail::valueless) {
    // TODO
  }

  template <class T>
    requires requires {
      requires sizeof...(Ts) > 0;
      requires !std::is_same_v<std::remove_cvref_t<T>, rvariant>;
      requires !detail::is_ttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_type_t>;
      requires !detail::is_nttp_specialization_of_v<std::remove_cvref_t<T>, std::in_place_index_t>;
      requires std::is_constructible_v<detail::pack_indexing_t<detail::accepted_index_v<T, rvariant>, Ts...>, T>;
    }
  constexpr rvariant(T&& x) noexcept(std::is_nothrow_constructible_v<detail::pack_indexing_t<detail::accepted_index_v<T, rvariant>, Ts...>, T>)
      : rvariant(std::in_place_index<detail::accepted_index_v<T, rvariant>>, std::forward<T>(x)) {}

  template <class T, class... Args>
    requires detail::exactly_once_v<T, Ts...> && std::is_constructible_v<T, Args...>
  constexpr explicit rvariant(std::in_place_type_t<T>, Args&&... args)
      : rvariant(std::in_place_index<detail::accepted_index_v<T, rvariant>>, std::forward<Args>(args)...) {}

  template <class T, class U, class... Args>
    requires detail::exactly_once_v<T, Ts...> && std::is_constructible_v<T, std::initializer_list<U>&, Args...>
  constexpr explicit rvariant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
      : rvariant(std::in_place_index<detail::accepted_index_v<T, rvariant>>, il, std::forward<Args>(args)...) {}

  template <std::size_t I, class... Args>
    requires(I < sizeof...(Ts)) && std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, Args...>
  constexpr explicit rvariant(std::in_place_index_t<I>, Args&&... args) : storage_(std::in_place_index<I>, std::forward<Args>(args)...), index_(I) {}

  template <std::size_t I, class U, class... Args>
    requires(I < sizeof...(Ts)) && std::is_constructible_v<detail::pack_indexing_t<I, Ts...>, std::initializer_list<U>&, Args...>
  constexpr explicit rvariant(std::in_place_index_t<I>, std::initializer_list<U> il, Args&&... args)
      : storage_(std::in_place_index<I>, il, std::forward<Args>(args)...), index_(I) {}

  constexpr ~rvariant() = default;

  constexpr bool valueless_by_exception() const noexcept { return index_ == variant_npos; }
  constexpr std::size_t index() const noexcept { return index_; }

  template <std::size_t I, class Variant>
  friend constexpr decltype(auto) detail::get_impl(Variant&&) noexcept;

private:
  detail::variadic_union<(std::is_trivially_destructible_v<Ts> && ...), Ts...> storage_;
  std::size_t index_ = variant_npos;
};

template <std::size_t I, class... Ts>
constexpr variant_alternative_t<I, rvariant<Ts...>>& get(rvariant<Ts...>& var) {
  if (I != var.index()) throw std::bad_variant_access{};
  return detail::get_impl<I>(var);
}

template <std::size_t I, class... Ts>
constexpr variant_alternative_t<I, rvariant<Ts...>>&& get(rvariant<Ts...>&& var) {
  if (I != var.index()) throw std::bad_variant_access{};
  return detail::get_impl<I>(std::move(var));
}

template <std::size_t I, class... Ts>
constexpr const variant_alternative_t<I, rvariant<Ts...>>& get(const rvariant<Ts...>& var) {
  if (I != var.index()) throw std::bad_variant_access{};
  return detail::get_impl<I>(var);
}

template <std::size_t I, class... Ts>
constexpr const variant_alternative_t<I, rvariant<Ts...>>&& get(const rvariant<Ts...>&& var) {
  if (I != var.index()) throw std::bad_variant_access{};
  return detail::get_impl<I>(std::move(var));
}

}  // namespace yk

#endif  // YK_RVARIANT_HPP
