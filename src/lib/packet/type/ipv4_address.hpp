#ifndef _LIB_PACKET_TYPE_IPV4_ADDRESS_HPP_
#define _LIB_PACKET_TYPE_IPV4_ADDRESS_HPP_

#include "packet/type/endian.hpp"

namespace libpacket::type {

class ipv4_address final : public endian::field<4>
{
public:
    constexpr ipv4_address() noexcept
        : endian::field<4>()
    {}

    ipv4_address(const char*);
    ipv4_address(const std::string&);
    ipv4_address(std::initializer_list<uint8_t>);
    ipv4_address(const uint8_t[4]) noexcept;
    ipv4_address(uint32_t) noexcept;
    ipv4_address(const ipv4_address&) = default;

    /***
     * Unary operators
     ***/

    ipv4_address operator+() const { return (load<uint32_t>()); }

    ipv4_address operator-() const { return -(load<uint32_t>()); }

    /***
     * Increment/decrement operators
     ***/

    /* Prefix */
    constexpr ipv4_address operator++()
    {
        this->store(load<uint32_t>() + 1);
        return (*this);
    }

    constexpr ipv4_address operator--()
    {
        this->store(load<uint32_t>() - 1);
        return (*this);
    }

    /* Postfix */
    constexpr ipv4_address operator++(int)
    {
        auto tmp = ipv4_address(*this);
        operator++();
        return (tmp);
    }

    constexpr ipv4_address operator--(int)
    {
        auto tmp = ipv4_address(*this);
        operator--();
        return (tmp);
    }

    /***
     * Arithmetic assignment operators
     ***/

    constexpr ipv4_address& operator=(const ipv4_address& rhs)
    {
        this->store(rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator+=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() + rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator-=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() - rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator*=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() * rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator/=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() / rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator%=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() % rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator&=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() & rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator|=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() | rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator^=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() ^ rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator<<=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() << rhs.load<uint32_t>());
        return (*this);
    }

    constexpr ipv4_address& operator>>=(const ipv4_address& rhs)
    {
        this->store(load<uint32_t>() >> rhs.load<uint32_t>());
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator=(const T& rhs)
    {
        this->store(rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator+=(const T& rhs)
    {
        this->store(load<uint32_t>() + rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator-=(const T& rhs)
    {
        this->store(load<uint32_t>() - rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator*=(const T& rhs)
    {
        this->store(load<uint32_t>() * rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator/=(const T& rhs)
    {
        this->store(load<uint32_t>() / rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator%=(const T& rhs)
    {
        this->store(load<uint32_t>() % rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator&=(const T& rhs)
    {
        this->store(load<uint32_t>() & rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator|=(const T& rhs)
    {
        this->store(load<uint32_t>() | rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator^=(const T& rhs)
    {
        this->store(load<uint32_t>() ^ rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator<<=(const T& rhs)
    {
        this->store(load<uint32_t>() << rhs);
        return (*this);
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_arithmetic_v<T>, ipv4_address&>
    operator>>=(const T& rhs)
    {
        this->store(load<uint32_t>() >> rhs);
        return (*this);
    }
};

std::string to_string(const ipv4_address&);

inline std::ostream& operator<<(std::ostream& os, const ipv4_address& mac)
{
    os << to_string(mac);
    return (os);
}

template <typename T>
inline constexpr ipv4_address operator+(ipv4_address lhs, const T& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator-(ipv4_address lhs, const T& rhs)
{
    lhs -= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator*(ipv4_address lhs, const T& rhs)
{
    lhs *= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator/(ipv4_address lhs, const T& rhs)
{
    lhs /= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator%(ipv4_address lhs, const T& rhs)
{
    lhs %= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator&(ipv4_address lhs, const T& rhs)
{
    lhs &= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator|(ipv4_address lhs, const T& rhs)
{
    lhs |= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator^(ipv4_address lhs, const T& rhs)
{
    lhs ^= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator<<(ipv4_address lhs, const T& rhs)
{
    lhs <<= rhs;
    return (lhs);
}

template <typename T>
inline constexpr ipv4_address operator>>(ipv4_address lhs, const T& rhs)
{
    lhs >>= rhs;
    return (lhs);
}

bool is_loopback(const ipv4_address&);
bool is_multicast(const ipv4_address&);

} // namespace libpacket::type

#endif /* _LIB_PACKET_TYPE_IPV4_ADDRESS_HPP_ */
