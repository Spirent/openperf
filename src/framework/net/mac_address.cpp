#include <cstring>
#include <stdexcept>

#include "net/mac_address.hpp"

namespace openperf {
namespace net {

mac_address::mac_address()
    : m_octets{0, 0, 0, 0, 0, 0}
{}

mac_address::mac_address(const std::string& input)
{
    /* Sanity check input */
    static const std::string delimiters("-:.");
    static const std::string hex_digits = "0123456789abcdefABCDEF" + delimiters;
    for (size_t i = 0; i < input.length(); i++) {
        if (hex_digits.find(input.at(i)) == std::string::npos) {
            throw std::runtime_error(std::string(&input.at(i))
                                     + " is not a valid hexadecimal character");
        }
    }

    size_t beg = 0, pos = 0, idx = 0;
    while ((beg = input.find_first_not_of(delimiters, pos))
           != std::string::npos) {
        if (idx == 6) {
            throw std::runtime_error("Too many octets in MAC address " + input);
        }

        pos = input.find_first_of(delimiters, beg + 1);
        auto value =
            std::strtol(input.substr(beg, pos - beg).c_str(), nullptr, 16);
        if (value < 0 || 0xff < value) {
            throw std::runtime_error("MAC address octet "
                                     + input.substr(beg, pos - beg)
                                     + " is not between 0x00 and 0xff");
        }

        m_octets[idx++] = value;
    }
    if (idx != 6) {
        throw std::runtime_error("Too few octets in MAC address " + input);
    }
}

mac_address::mac_address(std::initializer_list<uint8_t> data)
{
    if (data.size() != 6) { throw std::runtime_error("6 items required"); }
    size_t idx = 0;
    for (auto value : data) { m_octets[idx++] = value; }
}

mac_address::mac_address(const uint8_t data[6])
    : m_octets{data[0], data[1], data[2], data[3], data[4], data[5]}
{}

bool mac_address::is_unicast() const
{
    return ((m_octets[0] & group_addr_bit) == 0);
}

bool mac_address::is_multicast() const
{
    return ((m_octets[0] & group_addr_bit) == group_addr_bit);
}

bool mac_address::is_broadcast() const
{
    return (m_octets[0] == 0xff && m_octets[1] == 0xff && m_octets[2] == 0xff
            && m_octets[3] == 0xff && m_octets[4] == 0xff
            && m_octets[5] == 0xff);
}

bool mac_address::is_globally_unique() const
{
    return ((m_octets[0] & local_admin_bit) == 0);
}

bool mac_address::is_local_admin() const
{
    return ((m_octets[0] & local_admin_bit) == local_admin_bit);
}

uint8_t mac_address::operator[](size_t idx) const
{
    if (idx > 5) {
        throw std::out_of_range(std::to_string(idx)
                                + " is not between 0 and 5");
    }
    return m_octets[idx];
}

const uint8_t* mac_address::data() const { return (&m_octets[0]); }

std::string to_string(const mac_address& mac)
{
    char buffer[18];
    sprintf(buffer,
            "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0],
            mac[1],
            mac[2],
            mac[3],
            mac[4],
            mac[5]);
    return (std::string(buffer));
}

int compare(const mac_address& lhs, const mac_address& rhs)
{
    return std::memcmp(lhs.m_octets, rhs.m_octets, 6);
}

} // namespace net
} // namespace openperf
