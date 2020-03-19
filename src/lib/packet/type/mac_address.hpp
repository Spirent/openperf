#ifndef _LIB_PACKET_TYPE_MAC_ADDRESS_HPP_
#define _LIB_PACKET_TYPE_MAC_ADDRESS_HPP_

#include "packet/type/endian.hpp"

namespace libpacket::type {

class mac_address final : public endian::field<6>
{
public:
    constexpr mac_address() noexcept
        : endian::field<6>()
    {}

    // mac_address(const char*);
    mac_address(std::string_view);
    mac_address(std::initializer_list<uint8_t>);
    mac_address(const uint8_t[6]) noexcept;
    mac_address(uint64_t) noexcept;

    /***
     * Unary operators
     ***/

    mac_address operator+() const { return (load<uint64_t>()); }

    mac_address operator-() const { return -(load<uint64_t>()); }

    /***
     * Increment/decrement operators
     ***/

    /* Prefix */
    constexpr mac_address operator++()
    {
        this->store(load<uint64_t>() + 1);
        return (*this);
    }

    constexpr mac_address operator--()
    {
        this->store(load<uint64_t>() - 1);
        return (*this);
    }

    /* Postfix */
    constexpr mac_address operator++(int)
    {
        auto tmp = mac_address(*this);
        operator++();
        return (tmp);
    }

    constexpr mac_address operator--(int)
    {
        auto tmp = mac_address(*this);
        operator--();
        return (tmp);
    }

    /***
     * Arithmetic assignment operators
     ***/

    constexpr mac_address& operator=(const mac_address& rhs)
    {
        this->store(rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator+=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() + rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator-=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() - rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator*=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() * rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator/=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() / rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator%=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() % rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator&=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() & rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator|=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() | rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator^=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() ^ rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator<<=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() << rhs.load<uint64_t>());
        return (*this);
    }

    constexpr mac_address& operator>>=(const mac_address& rhs)
    {
        this->store(load<uint64_t>() >> rhs.load<uint64_t>());
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator=(const T& rhs)
    {
        this->store(rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator+=(const T& rhs)
    {
        this->store(load<uint64_t>() + rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator-=(const T& rhs)
    {
        this->store(load<uint64_t>() - rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator*=(const T& rhs)
    {
        this->store(load<uint64_t>() * rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator/=(const T& rhs)
    {
        this->store(load<uint64_t>() / rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator%=(const T& rhs)
    {
        this->store(load<uint64_t>() % rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator&=(const T& rhs)
    {
        this->store(load<uint64_t>() & rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator|=(const T& rhs)
    {
        this->store(load<uint64_t>() | rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator^=(const T& rhs)
    {
        this->store(load<uint64_t>() ^ rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator<<=(const T& rhs)
    {
        this->store(load<uint64_t>() << rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, mac_address&>
    operator>>=(const T& rhs)
    {
        this->store(load<uint64_t>() >> rhs);
        return (*this);
    }
};

std::string to_string(const mac_address&);

inline std::ostream& operator<<(std::ostream& os, const mac_address& mac)
{
    os << to_string(mac);
    return (os);
}

template <typename T>
inline constexpr mac_address operator+(mac_address lhs, const T& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator-(mac_address lhs, const T& rhs)
{
    lhs -= rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator*(mac_address lhs, const T& rhs)
{
    lhs *= rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator/(mac_address lhs, const T& rhs)
{
    lhs /= rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator%(mac_address lhs, const T& rhs)
{
    lhs %= rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator&(mac_address lhs, const T& rhs)
{
    lhs &= rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator|(mac_address lhs, const T& rhs)
{
    lhs |= rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator^(mac_address lhs, const T& rhs)
{
    lhs ^= rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator<<(mac_address lhs, const T& rhs)
{
    lhs <<= rhs;
    return (lhs);
}

template <typename T>
inline constexpr mac_address operator>>(mac_address lhs, const T& rhs)
{
    lhs >>= rhs;
    return (lhs);
}

bool is_unicast(const mac_address&);
bool is_multicast(const mac_address&);
bool is_broadcast(const mac_address&);
bool is_globally_unique(const mac_address&);
bool is_local_admin(const mac_address&);

} // namespace libpacket::type

namespace std {

template <> struct hash<libpacket::type::mac_address>
{
    size_t operator()(const libpacket::type::mac_address& mac) const
    {
        return (std::hash<uint64_t>{}(mac.load<uint64_t>()));
    }
};

} // namespace std

#endif /* _LIB_PACKET_TYPE_MAC_ADDRESS_HPP_ */
