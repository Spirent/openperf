#ifndef _LIB_PACKET_TYPE_ENDIAN_H_
#define _LIB_PACKET_TYPE_ENDIAN_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cstdint>
#include <string>

namespace libpacket::type::endian {

static_assert(CHAR_BIT == 8, "char is wrong size!");

enum class order {
    little = __ORDER_LITTLE_ENDIAN__,
    big = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
};

/***
 * Endian conversion routines
 ***/

/**
 * The compiler recognizes this fold expression as bswap on x86 platforms.
 */

template <class T, size_t... N>
constexpr T bswap_impl(T i, std::index_sequence<N...>)
{
    return (((static_cast<T>(i) >> N * CHAR_BIT
              & std::numeric_limits<uint8_t>::max())
             << (sizeof(T) - 1 - N) * CHAR_BIT)
            | ...);
}

/**
 * Template specialization for IPv6 addresses.  Since there is no native
 * instruction to byte swap 128 bits, the compiler needs some help optimizing
 * this swap.
 */

template <class T,
          size_t N0 = 0,
          size_t N1 = 1,
          size_t N2 = 2,
          size_t N3 = 3,
          size_t N4 = 4,
          size_t N5 = 5,
          size_t N6 = 6,
          size_t N7 = 7,
          size_t N8 = 8,
          size_t N9 = 9,
          size_t N10 = 10,
          size_t N11 = 11,
          size_t N12 = 12,
          size_t N13 = 13,
          size_t N14 = 14,
          size_t N15 = 15>
constexpr T bswap_impl(T i, std::make_index_sequence<16>)
{
    auto x = reinterpret_cast<uint64_t*>(std::addressof(i));
    return (static_cast<T>(bswap_impl(x[0], std::make_index_sequence<8>{}))
                << 64
            | bswap_impl(x[1], std::make_index_sequence<8>{}));
}

template <class T, class U = std::make_unsigned_t<T>>
constexpr std::enable_if_t<order::native == order::little, U> bswap(T value)
{
    return (bswap_impl<U>(value, std::make_index_sequence<sizeof(T)>{}));
}

template <class T, class U = std::make_unsigned_t<T>>
constexpr std::enable_if_t<order::native == order::big, U> bswap(T value)
{
    return (static_cast<U>(value));
}

/*
 * Fields store data in big-endian format.  Load/store operations will
 * always convert to/from native byte format.
 */
template <size_t Octets> struct field
{
    static constexpr size_t width = Octets;
    using container = std::array<uint8_t, width>;

    container octets;

    using value_type = typename container::value_type;
    using size_type = typename container::size_type;
    using reference = typename container::reference;
    using const_reference = typename container::const_reference;
    using pointer = typename container::pointer;
    using const_pointer = typename container::const_pointer;

    template <typename T>
    constexpr void store(const std::initializer_list<T>& value) noexcept
    {
        assert(Octets == value.size() * sizeof(T));

        auto ptr = reinterpret_cast<T*>(octets.data());
        std::transform(std::begin(value),
                       std::begin(value) + value.size(),
                       ptr,
                       [](const auto& x) { return (bswap(x)); });
    }

    template <typename T>
    constexpr std::enable_if_t<Octets <= sizeof(T), void>
    store(T value) noexcept
    {
        auto tmp =
            bswap(static_cast<T>(value << ((sizeof(T) - Octets) * CHAR_BIT)));
        auto ptr = reinterpret_cast<uint8_t*>(std::addressof(tmp));
        std::copy_n(ptr, Octets, octets.data());
    }

    template <typename T, size_type N>
    constexpr std::enable_if_t<Octets == (sizeof(T) * N), void>
    store(const T value[N]) noexcept
    {
        auto ptr = reinterpret_cast<T*>(octets.data());
        std::transform(
            value, value + N, ptr, [](const auto& x) { return (bswap(x)); });
    }

    field() = default;
    field(const std::initializer_list<value_type>& value) noexcept
    {
        store(value);
    }
    template <typename T> field(const T value) noexcept { store<T>(value); }
    template <typename T, size_t N> field(const T value[N]) noexcept
    {
        store<T, N>(value);
    }

    template <typename T>
    std::enable_if_t<Octets <= sizeof(T), T> load() const noexcept
    {
        const auto ptr = reinterpret_cast<const T*>(octets.data());
        return (bswap(*ptr) >> ((sizeof(T) - Octets) * CHAR_BIT));
    }

    template <typename T> T load(size_type idx) const
    {
        if (sizeof(T) * idx + 1 > Octets) {
            throw std::out_of_range(
                std::to_string(idx) + " is not between 0 and "
                + std::to_string((Octets + sizeof(T) - 1) / sizeof(T) - 1));
        }

        if (sizeof(T) * (idx + 1) <= Octets) {
            /* All required data is in the array */
            auto cursor = reinterpret_cast<const T*>(octets.data()) + idx;
            return (bswap(*cursor));
        } else {
            /* We need the last few octets of the array presented as type T */
            auto remainder = Octets - (sizeof(T) * idx);
            auto tmp = T{0};
            auto shift = 0;
            auto cursor = std::make_reverse_iterator(std::end(octets));
            std::for_each(cursor, cursor + remainder, [&](const auto& octet) {
                tmp |= (static_cast<T>(octet) << shift);
                shift += CHAR_BIT;
            });
            return (tmp);
        }
    }

    constexpr pointer data() noexcept { return (octets.data()); }

    constexpr const_pointer data() const noexcept { return (octets.data()); }

    constexpr bool empty() const noexcept { return (octets.empty()); }

    constexpr size_type size() const noexcept { return (octets.size()); }

    constexpr size_type max_size() const noexcept
    {
        return (octets.max_size());
    }

    constexpr const_reference at(size_type idx) const
    {
        if (!(idx < size())) {
            throw std::out_of_range(std::to_string(idx)
                                    + " is larger than field size "
                                    + std::to_string(size()));
        }
        return (octets[idx]);
    }

    constexpr reference at(size_type idx)
    {
        return (const_cast<reference>(
            const_cast<const endian::field<Octets>&>(*this).at(idx)));
    }

    constexpr reference operator[](size_type idx) noexcept
    {
        return (octets[idx]);
    }

    constexpr const_reference operator[](size_type idx) const noexcept
    {
        return (octets[idx]);
    }
};

/***
 * Comparison functions
 ***/

template <size_t Octets>
inline constexpr bool operator==(const field<Octets>& lhs,
                                 const field<Octets>& rhs)
{
    return (lhs.octets == rhs.octets);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator==(const field<Octets>& lhs, const T& rhs)
{
    return (lhs.template load<T>() == rhs);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator==(const T& rhs, const field<Octets>& lhs)
{
    return (rhs == lhs.template load<T>());
}

template <size_t Octets>
inline constexpr bool operator!=(const field<Octets>& lhs,
                                 const field<Octets>& rhs)
{
    return (lhs.octets != rhs.octets);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator!=(const field<Octets>& lhs, const T& rhs)
{
    return (lhs.template load<T>() != rhs);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator!=(const T& rhs, const field<Octets>& lhs)
{
    return (rhs != lhs.template load<T>());
}

template <size_t Octets>
inline constexpr bool operator<(const field<Octets>& lhs,
                                const field<Octets>& rhs)
{
    return (lhs.octets < rhs.octets);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator<(const field<Octets>& lhs, const T& rhs)
{
    return (lhs.template load<T>() < rhs);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator<(const T& rhs, const field<Octets>& lhs)
{
    return (rhs < lhs.template load<T>());
}

template <size_t Octets>
inline constexpr bool operator<=(const field<Octets>& lhs,
                                 const field<Octets>& rhs)
{
    return (lhs.octets <= rhs.octets);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator<=(const field<Octets>& lhs, const T& rhs)
{
    return (lhs.template load<T>() <= rhs);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator<=(const T& rhs, const field<Octets>& lhs)
{
    return (rhs <= lhs.template load<T>());
}

template <size_t Octets>
inline constexpr bool operator>(const field<Octets>& lhs,
                                const field<Octets>& rhs)
{
    return (lhs.octets > rhs.octets);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator>(const field<Octets>& lhs, const T& rhs)
{
    return (lhs.template load<T>() > rhs);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator>(const T& rhs, const field<Octets>& lhs)
{
    return (rhs > lhs.template load<T>());
}

template <size_t Octets>
inline constexpr bool operator>=(const field<Octets>& lhs,
                                 const field<Octets>& rhs)
{
    return (lhs.octets >= rhs.octets);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator>=(const field<Octets>& lhs, const T& rhs)
{
    return (lhs.template load<T>() >= rhs);
}

template <typename T, size_t Octets>
inline constexpr std::enable_if_t<std::is_integral_v<T> && Octets <= sizeof(T),
                                  bool>
operator>=(const T& rhs, const field<Octets>& lhs)
{
    return (rhs >= lhs.template load<T>());
}

template <typename T, size_t Octets = sizeof(T)> struct number : field<Octets>
{
    static_assert(
        Octets <= sizeof(T),
        "integral type must be at least as large as the number of octets");

    constexpr number() = default;
    number(T value) noexcept { field<Octets>::store(value); }

    T load() const noexcept { return (field<Octets>::template load<T>()); }

    /***
     * Unary operators
     ***/

    constexpr T operator+() const { return (load()); }

    constexpr T operator-() const { return -(load()); }

    /***
     * Increment/decrement operators
     ***/

    /* Prefix */
    constexpr number& operator++()
    {
        this->store(load() + 1);
        return (this);
    }

    constexpr number& operator--()
    {
        this->store(load() - 1);
        return (this);
    }

    /* Postfix */
    constexpr number operator++(int)
    {
        auto tmp = number(*this);
        operator++();
        return (tmp);
    }

    constexpr number operator--(int)
    {
        auto tmp = number(*this);
        operator--();
        return (tmp);
    }

    /***
     * Arithmetic assignment operators
     ***/

    template <typename U> constexpr number& operator=(const U& rhs)
    {
        this->store(rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator+=(const U& rhs)
    {
        this->store(load() + rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator-=(const U& rhs)
    {
        this->store(load() - rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator*=(const U& rhs)
    {
        this->store(load() * rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator/=(const U& rhs)
    {
        this->store(load() / rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator%=(const U& rhs)
    {
        this->store(load() % rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator&=(const U& rhs)
    {
        this->store(load() & rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator|=(const U& rhs)
    {
        this->store(load() | rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator^=(const U& rhs)
    {
        this->store(load() ^ rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator<<=(const U& rhs)
    {
        this->store(load() << rhs);
        return (*this);
    }

    template <typename U> constexpr number& operator>>=(const U& rhs)
    {
        this->store(load() >> rhs);
        return (*this);
    }
};

template <typename T, size_t Octets>
inline std::ostream& operator<<(std::ostream& os, number<T, Octets> n)
{
    os << n.load();
    return (os);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator+(number<T, Octets> lhs,
                                             const U& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator-(number<T, Octets> lhs,
                                             const U& rhs)
{
    lhs -= rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator*(number<T, Octets> lhs,
                                             const U& rhs)
{
    lhs *= rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator/(number<T, Octets> lhs,
                                             const U& rhs)
{
    lhs /= rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator%(number<T, Octets> lhs,
                                             const U& rhs)
{
    lhs %= rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator&(number<T, Octets> lhs,
                                             const U& rhs)
{
    lhs &= rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator|(number<T, Octets> lhs,
                                             const U& rhs)
{
    lhs |= rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator^(number<T, Octets> lhs,
                                             const U& rhs)
{
    lhs ^= rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator<<(number<T, Octets> lhs,
                                              const U& rhs)
{
    lhs <<= rhs;
    return (lhs);
}

template <typename T, size_t Octets, typename U>
inline constexpr number<T, Octets> operator>>(number<T, Octets> lhs,
                                              const U& rhs)
{
    lhs >>= rhs;
    return (lhs);
}

} // namespace libpacket::type::endian

#endif /* _LIB_PACKET_TYPE_ENDIAN_H_ */
