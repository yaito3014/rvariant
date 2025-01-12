#ifndef YK_RVARIANT_HPP
#define YK_RVARIANT_HPP

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include <climits>
#include <cstddef>

namespace yk {

template <class... Ts>
struct type_list {};

template <class... Ts>
class rvariant;

struct recursive_self {};

template <class T>
struct variant_size;

template <class T>
struct variant_size<const T> : variant_size<T> {};

template <class T>
inline constexpr std::size_t variant_size_v = variant_size<T>::value;

template <class... Ts>
struct variant_size<rvariant<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <std::size_t I, class V>
struct variant_alternative;

template <std::size_t I, class V>
struct variant_alternative<I, const V> : variant_alternative<I, V> {};

template <std::size_t I, class V>
using variant_alternative_t = typename variant_alternative<I, V>::type;

namespace detail {

template <std::size_t I, class... Ts>
struct pack_indexing;

template <std::size_t I, class... Ts>
using pack_indexing_t = typename pack_indexing<I, Ts...>::type;

template <class T, class... Ts>
struct pack_indexing<0, T, Ts...> {
  using type = T;
};

template <std::size_t I, class T, class... Ts>
struct pack_indexing<I, T, Ts...> : pack_indexing<I - 1, Ts...> {};

template <std::size_t I, class TL>
struct list_indexing;

template <std::size_t I, class TL>
using list_indexing_t = typename list_indexing<I, TL>::type;

template <std::size_t I, class... Ts>
struct list_indexing<I, type_list<Ts...>> : pack_indexing<I, Ts...> {};

template <class From, class To, class T>
struct replace {
  using type = T;
};

template <class From, class To, class T>
using replace_t = typename replace<From, To, T>::type;

template <class From, class To>
struct replace<From, To, From> {
  using type = To;
};

template <class From, class To, template <class...> class TT, class... Ts>
struct replace<From, To, TT<Ts...>> {
  using type = TT<replace_t<From, To, Ts>...>;
};

}  // namespace detail

template <std::size_t I, class... Ts>
struct variant_alternative<I, rvariant<Ts...>> {
  using type = detail::pack_indexing_t<I, detail::replace_t<recursive_self, rvariant<Ts...>, Ts>...>;
};

inline constexpr size_t variant_npos = -1;

namespace detail {

template <std::size_t Index, class T>
struct alternative {
  using value_type = T;
  static constexpr std::size_t index = Index;

  template <class... Args>
  constexpr explicit alternative(std::in_place_t, Args&&... args) : value(std::forward<Args>(args)...) {}

  T value;
};

template <bool trivially_destructible, std::size_t Index, class... Ts>
union variadic_union {
  variadic_union() noexcept = default;
};

template <bool trivially_destructible, std::size_t Index, class T, class... Ts>
union variadic_union<trivially_destructible, Index, T, Ts...> {
  constexpr variadic_union() noexcept : rest() {}

  template <class... Args>
  constexpr variadic_union(std::in_place_index_t<0>, Args&&... args) : first(std::in_place, std::forward<Args>(args)...) {}

  template <std::size_t I, class... Args>
  constexpr variadic_union(std::in_place_index_t<I>, Args&&... args) : rest(std::in_place_index<I - 1>, std::forward<Args>(args)...) {}

  ~variadic_union() = default;

  constexpr ~variadic_union()
    requires(!trivially_destructible)
  {
    //
  }

  alternative<Index, T> first;
  variadic_union<trivially_destructible, Index + 1, Ts...> rest;
};

template <class Union>
constexpr auto&& get_alternative(Union&& u, std::in_place_index_t<0>) noexcept {
  return std::forward<Union>(u).first;
}

template <std::size_t I, class Union>
constexpr auto&& get_alternative(Union&& u, std::in_place_index_t<I>) noexcept {
  return get_alternative(std::forward<Union>(u).rest, std::in_place_index<I - 1>);
}

template <std::size_t I, class Variant>
constexpr auto&& raw_get(Variant&& v) noexcept {
  return get_alternative(std::forward<Variant>(v).storage_, std::in_place_index<I>);
}

template <std::size_t Size>
using select_index_t = std::conditional_t<(Size <= 1 << (sizeof(unsigned char) * CHAR_BIT)), unsigned char, unsigned short>;

}  // namespace detail

template <size_t I, class... Types>
constexpr variant_alternative_t<I, rvariant<Types...>>& get(rvariant<Types...>& v) {
  if (v.index() != I) throw std::bad_variant_access{};
  return detail::raw_get<I>(v).value;
}
template <size_t I, class... Types>
constexpr variant_alternative_t<I, rvariant<Types...>>&& get(rvariant<Types...>&& v) {
  if (v.index() != I) throw std::bad_variant_access{};
  return detail::raw_get<I>(std::move(v)).value;
}
template <size_t I, class... Types>
constexpr const variant_alternative_t<I, rvariant<Types...>>& get(const rvariant<Types...>& v) {
  if (v.index() != I) throw std::bad_variant_access{};
  return detail::raw_get<I>(v).value;
}
template <size_t I, class... Types>
constexpr const variant_alternative_t<I, rvariant<Types...>>&& get(const rvariant<Types...>&& v) {
  if (v.index() != I) throw std::bad_variant_access{};
  return detail::raw_get<I>(std::move(v)).value;
}

namespace detail {

template <class Visitor, bool TD, std::size_t Index, class... Ts, std::size_t... Is>
constexpr auto make_farray_for_raw_visit_at(std::index_sequence<Is...>) {
  return std::array{
      [](Visitor&& vi, variadic_union<TD, Index, Ts...>& vu) { std::invoke(std::forward<Visitor>(vi), get_alternative(vu, std::in_place_index<Is>)); }...};
}

template <class Visitor, bool TD, std::size_t Index, class... Ts>
constexpr decltype(auto) raw_visit_at(Visitor&& vis, std::size_t index, variadic_union<TD, Index, Ts...>& varuni) {
  constexpr auto VariantSize = sizeof...(Ts);
  constexpr auto farray = make_farray_for_raw_visit_at<Visitor, TD, Index, Ts...>(std::make_index_sequence<VariantSize>{});
  return farray[index](std::forward<Visitor>(vis), varuni);
}

template <class Visitor, class Variant, std::size_t... Is>
constexpr auto make_farray_for_raw_visit(std::index_sequence<Is...>) {
  return std::array{+[](Visitor&& vi, Variant&& va) { std::invoke(std::forward<Visitor>(vi), raw_get<Is>(va)); }...};
}

template <class Visitor, class Variant>
constexpr decltype(auto) raw_visit(Visitor&& vis, Variant&& var) {
  constexpr auto VariantSize = variant_size_v<std::remove_cvref_t<Variant>>;
  constexpr auto farray = make_farray_for_raw_visit<Visitor, Variant>(std::make_index_sequence<VariantSize>{});
  return farray[var.index()](std::forward<Visitor>(vis), std::forward<Variant>(var));
}

template <class R, class Visitor, class Variant, std::size_t... Is>
constexpr auto make_farray_for_visit(std::index_sequence<Is...>) {
  return std::array{+[](Visitor&& vi, Variant&& va) -> R { return std::invoke(std::forward<Visitor>(vi), get<Is>(std::forward<Variant>(va))); }...};
}

}  // namespace detail

template <class Visitor, class Variant>
constexpr decltype(auto) visit(Visitor&& vis, Variant&& var) {
  constexpr auto VariantSize = variant_size_v<std::remove_cvref_t<Variant>>;
  using R = std::invoke_result_t<Visitor, variant_alternative_t<0, std::remove_cvref_t<Variant>>>;
  constexpr auto farray = detail::make_farray_for_visit<R, Visitor, Variant>(std::make_index_sequence<VariantSize>{});
  return farray[var.index()](std::forward<Visitor>(vis), std::forward<Variant>(var));
}

template <class... Ts>
class rvariant {
  static_assert(sizeof...(Ts) > 0, "rvariant must have at least one alternative");

  using storage_t = detail::variadic_union<(std::is_trivially_destructible_v<Ts> && ...), 0, detail::replace_t<recursive_self, rvariant, Ts>...>;
  using first_type = variant_alternative_t<0, rvariant>;

  using specified_types = type_list<Ts...>;
  using transformed_types = type_list<detail::replace_t<recursive_self, rvariant, Ts>...>;

public:
  constexpr rvariant() noexcept(std::is_nothrow_default_constructible_v<first_type>)
    requires std::is_default_constructible_v<first_type>
      : storage_(std::in_place_index<0>), index_(0) {}

  template <std::size_t I, class... Args>
    requires(I < sizeof...(Ts)) && std::constructible_from<detail::list_indexing_t<I, transformed_types>, Args...>
  constexpr explicit rvariant(std::in_place_index_t<I>, Args&&... args) : storage_(std::in_place_index<I>, std::forward<Args>(args)...), index_(I) {}

  template <std::size_t I, class T, class... Args>
    requires(I < sizeof...(Ts)) && std::constructible_from<detail::list_indexing_t<I, transformed_types>, std::initializer_list<T>&, Args...>
  constexpr explicit rvariant(std::in_place_index_t<I>, std::initializer_list<T> il, Args&&... args)
      : storage_(std::in_place_index<I>, il, std::forward<Args>(args)...), index_(I) {}

  constexpr rvariant(const rvariant& other) : index_(other.index_) {
    detail::raw_visit([this](auto&& alt) { std::construct_at(&storage_, std::in_place_index<std::remove_cvref_t<decltype(alt)>::index>, alt.value); }, other);
  }

  constexpr rvariant(rvariant&& other) : index_(other.index_) {
    detail::raw_visit(
        [this](auto&& alt) { std::construct_at(&storage_, std::in_place_index<std::remove_cvref_t<decltype(alt)>::index>, std::move(alt.value)); },
        std::move(other));
  }

  constexpr rvariant& operator=(const rvariant& other) {
    if (index_ == other.index_) {
      detail::raw_visit(
          [this](auto&& alt) {
            constexpr auto OtherIndex = std::remove_cvref_t<decltype(alt)>::index;
            detail::raw_get<OtherIndex>(*this).value = alt.value;
          },
          other);
      return *this;
    } else {
      auto backup = std::make_unique<storage_t>();

      backup->~storage_t();

      detail::raw_visit_at([&](auto&& alt) { std::construct_at(backup.get(), std::in_place_index<std::remove_cvref_t<decltype(alt)>::index>, alt.value); },
                           index_, storage_);

      detail::raw_visit([](auto&& alt) { std::destroy_at(std::addressof(alt)); }, *this);

      try {
        detail::raw_visit(
            [this](auto&& alt) {
              constexpr auto OtherIndex = std::remove_cvref_t<decltype(alt)>::index;
              std::construct_at(&storage_, std::in_place_index<OtherIndex>, alt.value);
            },
            other);
        index_ = other.index_;
        return *this;
      } catch (...) {
        detail::raw_visit_at([this](auto&& alt) { std::construct_at(&storage_, std::in_place_index<std::remove_cvref_t<decltype(alt)>::index>, alt.value); },
                             index_, *backup);
        throw;
      }
    }
  }

  constexpr rvariant& operator=(rvariant&& other) {
    if (index_ == other.index_) {
      detail::raw_visit(
          [this](auto&& alt) {
            constexpr auto OtherIndex = std::remove_cvref_t<decltype(alt)>::index;
            detail::raw_get<OtherIndex>(*this).value = std::move(alt.value);
          },
          std::move(other));
      return *this;
    } else {
      auto backup = std::make_unique<storage_t>();

      backup->~storage_t();

      detail::raw_visit_at([&](auto&& alt) { std::construct_at(backup.get(), std::in_place_index<std::remove_cvref_t<decltype(alt)>::index>, alt.value); },
                           index_, storage_);

      detail::raw_visit([](auto&& alt) { std::destroy_at(std::addressof(alt)); }, *this);

      try {
        detail::raw_visit(
            [this](auto&& alt) {
              constexpr auto OtherIndex = std::remove_cvref_t<decltype(alt)>::index;
              std::construct_at(&storage_, std::in_place_index<OtherIndex>, std::move(alt.value));
            },
            std::move(other));
        index_ = other.index_;
        return *this;
      } catch (...) {
        detail::raw_visit_at([this](auto&& alt) { std::construct_at(&storage_, std::in_place_index<std::remove_cvref_t<decltype(alt)>::index>, alt.value); },
                             index_, *backup);
        throw;
      }
    }
  }

  ~rvariant() = default;

  constexpr ~rvariant()
    requires(!(std::is_trivially_destructible_v<Ts> && ...))
  {
    detail::raw_visit([](auto&& alt) { std::destroy_at(std::addressof(alt)); }, *this);
  }

  constexpr std::size_t index() const noexcept { return index_; }

  template <std::size_t I, class rvariant>
  friend constexpr auto&& detail::raw_get(rvariant&& v) noexcept;

private:
  storage_t storage_;
  detail::select_index_t<sizeof...(Ts)> index_;
};

}  // namespace yk

#endif  // YK_RVARIANT_HPP
