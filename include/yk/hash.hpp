#ifndef YK_HASH_HPP
#define YK_HASH_HPP

// Copyright 2025 Nana Sakisaka
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <yk/core/hash.hpp>

#include <functional>
#include <bit>

#include <cstddef>
#include <cstdint>

// https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-03#section-2
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59406

namespace yk {

namespace detail {

template<std::size_t Base>
struct FNV_hash_numbers;

template<>
struct FNV_hash_numbers<4>
{
    static constexpr std::size_t FNV_offset_basis = 2166136261uz;
    static constexpr std::size_t FNV_prime = 16777619uz;
};

template<>
struct FNV_hash_numbers<8>
{
    static constexpr std::size_t FNV_offset_basis = 14695981039346656037uz;
    static constexpr std::size_t FNV_prime = 1099511628211uz;
};

template<std::size_t Base>
class FNV_hash_impl
{
    using N = FNV_hash_numbers<Base>;

public:
    [[nodiscard]] static constexpr std::size_t FNV1a(unsigned char const* const data, std::size_t count) noexcept
    {
        std::size_t hash = N::FNV_offset_basis;
        for (std::size_t i = 0; i < count; ++i) {
            hash ^= static_cast<std::size_t>(data[i]);
            hash *= N::FNV_prime;
        }
        return hash;
    }

    template<std::size_t count>
    [[nodiscard]] static constexpr std::size_t FNV1a(unsigned char const* const data) noexcept
    {
        std::size_t hash = N::FNV_offset_basis;
        for (std::size_t i = 0; i < count; ++i) {
            hash ^= static_cast<std::size_t>(data[i]);
            hash *= N::FNV_prime;
        }
        return hash;
    }

    template<class T>
    [[nodiscard]] static constexpr std::size_t hash(T const& t) noexcept
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if consteval {
            struct Storage
            {
                alignas(T) unsigned char data[sizeof(T)];
            };
            auto const storage = std::bit_cast<Storage>(t);
            return FNV_hash_impl::FNV1a<sizeof(T)>(storage.data);

        } else {
            return FNV_hash_impl::FNV1a<sizeof(T)>(reinterpret_cast<unsigned char const*>(std::addressof(t)));
        }
    }
};

} // detail

template<std::size_t Base = sizeof(std::size_t)>
using FNV_hash = detail::FNV_hash_impl<Base>;

namespace detail {

template<std::size_t Base>
constexpr std::size_t hash_mix(std::size_t) noexcept = delete;

// https://jonkagstrom.com/mx3/mx3_rev2.html

template<>
[[nodiscard]] constexpr std::size_t hash_mix<4>(std::size_t x) noexcept
{
    x ^= x >> 16;
    x *= 0x21f0aaaduz;
    x ^= x >> 15;
    x *= 0x735a2d97uz;
    x ^= x >> 15;
    return x;
}

template<>
[[nodiscard]] constexpr std::size_t hash_mix<8>(std::size_t x) noexcept
{
    x ^= x >> 32;
    x *= 0xe9846af9b1a615duz;
    x ^= x >> 32;
    x *= 0xe9846af9b1a615duz;
    x ^= x >> 28;
    return x;
}

} // detail

// https://github.com/boostorg/container_hash/blob/5d8b8ac2b9d9d7cb3818f88fd7e6372e5f072ff5/include/boost/container_hash/hash.hpp#L472C53-L472C63
// https://softwareengineering.stackexchange.com/questions/402542/where-do-magic-hashing-constants-like-0x9e3779b9-and-0x9e3779b1-come-from

template<class T>
[[nodiscard]] constexpr std::size_t hash_combine(std::size_t const seed, T const& v) noexcept
{
    static_assert(core::is_hash_enabled_v<T>);
    return detail::hash_mix<sizeof(std::size_t)>(
        seed + 0x9e3779b97f4a7c55uz + std::hash<T>{}(v)
    );
}

} // yk

#endif
