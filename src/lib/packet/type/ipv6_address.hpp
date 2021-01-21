#ifndef _LIB_PACKET_TYPE_IPV6_ADDRESS_HPP_
#define _LIB_PACKET_TYPE_IPV6_ADDRESS_HPP_

#include <cassert>

#include "packet/type/endian.hpp"

namespace libpacket::type {

namespace detail {

/**
 * Using 128 bit types makes arithmetic operations on IPv6 addresses
 * much easier to implement.
 */

static_assert(std::is_integral_v<__int128>, "128 bit types must be integral");
using uint128_t = unsigned __int128;

} // namespace detail

class ipv6_address final : public endian::field<16>
{
public:
    constexpr ipv6_address() noexcept
        : endian::field<16>()
    {}

    ipv6_address(const char*);
    ipv6_address(const std::string&);
    ipv6_address(std::initializer_list<uint8_t> data);
    ipv6_address(const uint8_t[16]) noexcept;
    ipv6_address(const uint32_t[4]) noexcept;
    ipv6_address(uint64_t) noexcept;
    ipv6_address(std::pair<uint64_t, uint64_t>) noexcept;
    ipv6_address(const ipv6_address&) = default;

    /***
     * Unary operators
     ***/

    ipv6_address operator+() const
    {
        return (std::make_pair(load<uint64_t>(0), load<uint64_t>(1)));
    }

    ipv6_address operator-() const
    {
        return (std::make_pair(-load<uint64_t>(0), -load<uint64_t>(1)));
    }

    /***
     * Increment/decrement operators
     ***/

    /* Prefix */
    constexpr ipv6_address operator++()
    {
        this->store(load<detail::uint128_t>() + 1);
        return (*this);
    }

    constexpr ipv6_address operator--()
    {
        this->store(load<detail::uint128_t>() - 1);
        return (*this);
    }

    /* Postfix */
    constexpr ipv6_address operator++(int)
    {
        auto tmp = ipv6_address(*this);
        operator++();
        return (tmp);
    }

    constexpr ipv6_address operator--(int)
    {
        auto tmp = ipv6_address(*this);
        operator--();
        return (tmp);
    }

    /***
     * Arithmetic assignment operators
     ***/

    constexpr ipv6_address& operator=(const ipv6_address& rhs)
    {
        this->store(rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator+=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() + rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator-=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() - rhs.load<detail::uint128_t>());
        return (*this);
    }

    ipv6_address& operator*=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() * rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator/=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() / rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator%=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() % rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator&=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() & rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator|=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() | rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator^=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() ^ rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator<<=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() << rhs.load<detail::uint128_t>());
        return (*this);
    }

    constexpr ipv6_address& operator>>=(const ipv6_address& rhs)
    {
        this->store(load<detail::uint128_t>() >> rhs.load<detail::uint128_t>());
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator=(const T& rhs)
    {
        this->store(rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator+=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() + rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator-=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() - rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator*=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() * rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator/=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() / rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator%=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() % rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator&=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() & rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator|=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() | rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator^=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() ^ rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator<<=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() << rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv6_address&>
    operator>>=(const T& rhs)
    {
        this->store(load<detail::uint128_t>() >> rhs);
        return (*this);
    }
};

std::string to_string(const ipv6_address&);

inline std::ostream& operator<<(std::ostream& os, const ipv6_address& addr)
{
    os << to_string(addr);
    return (os);
}

template <typename T>
inline constexpr ipv6_address operator+(ipv6_address lhs, const T& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv6_address operator-(ipv6_address lhs, const T& rhs)
{
    lhs -= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv6_address operator*(ipv6_address lhs, const T& rhs)
{
    lhs *= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv6_address operator/(ipv6_address lhs, const T& rhs)
{
    lhs /= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv6_address operator&(ipv6_address lhs, const T& rhs)
{
    lhs &= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv6_address operator|(ipv6_address lhs, const T& rhs)
{
    lhs |= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv6_address operator^(ipv6_address lhs, const T& rhs)
{
    lhs ^= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv6_address operator<<(ipv6_address lhs, const T& rhs)
{
    lhs <<= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv6_address operator>>(ipv6_address lhs, const T& rhs)
{
    lhs >>= rhs;
    return (lhs);
}

bool is_loopback(const ipv6_address&);
bool is_multicast(const ipv6_address&);
bool is_linklocal(const ipv6_address&);

} // namespace libpacket::type

namespace std {

template <> struct hash<libpacket::type::ipv6_address>
{
    size_t operator()(const libpacket::type::ipv6_address& ip) const
    {
        return (std::hash<uint64_t>{}(ip.load<uint64_t>(0))
                ^ std::hash<uint64_t>{}(ip.load<uint64_t>(1)));
    }
};

} // namespace std

#endif /* _LIB_PACKET_TYPE_IPV6_ADDRESS_HPP_ */
