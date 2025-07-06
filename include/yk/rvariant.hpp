#ifndef YK_RVARIANT_HPP
#define YK_RVARIANT_HPP

#include "yk/detail/pack_indexing.hpp"

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
struct fun {
  std::integral_constant<std::size_t, I> operator()()
    requires requires { []() { Dest x[] = {std::declval<Source>()}; }(); };
};

}  // namespace detail

template <class... Ts>
class rvariant {
public:
  constexpr rvariant() noexcept(std::is_nothrow_default_constructible_v<detail::pack_indexing_t<0, Ts...>>)
    requires std::is_default_constructible_v<detail::pack_indexing_t<0, Ts...>>
      : storage_{}, index_(0) {}

  constexpr rvariant(const rvariant&)
    requires detail::all_trivially_copy_constructible<Ts...>
  = default;

  constexpr rvariant(const rvariant&)
    requires(!std::is_copy_constructible_v<Ts> || ...)
  = delete;

  constexpr rvariant(const rvariant& other)
    requires detail::all_copy_constructible<Ts...>
      : storage_(other.storage_), index_(other.index_) {}

  constexpr rvariant(rvariant&&)
    requires detail::all_trivially_move_constructible<Ts...>
  = default;

  constexpr rvariant(rvariant&& other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
    requires detail::all_move_constructible<Ts...>
      : storage_(std::move(other.storage_)) {}

  // template <class T>
  //   requires requires {
  //     requires sizeof...(Ts) > 0;
  //   }
  // constexpr rvariant(T&& x) {}

  constexpr ~rvariant() = default;

  constexpr bool valueless_by_exception() const noexcept { return index_ == variant_npos; }
  constexpr std::size_t index() const noexcept { return index_; }

private:
  std::variant<Ts...> storage_;
  std::size_t index_ = variant_npos;
};

}  // namespace yk

#endif  // YK_RVARIANT_HPP
