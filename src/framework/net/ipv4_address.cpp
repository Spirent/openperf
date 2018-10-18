#include <stdexcept>

#include <arpa/inet.h>

#include "net/ipv4_address.h"

namespace icp {
namespace net {

ipv4_address::ipv4_address()
    : m_addr(0)
{}

ipv4_address::ipv4_address(const std::string& input)
{
    if (inet_pton(AF_INET, input.c_str(), &m_addr) == 0) {
        throw std::runtime_error("Invalid IPv4 address: " + input);
    }
}

ipv4_address::ipv4_address(std::initializer_list<uint8_t> data)
{
    if (data.size() != 4) {
        throw std::runtime_error("4 items required");
    }
    size_t idx = 0;
    for (auto value : data) {
        m_octets[idx++] = value;
    }
}

ipv4_address::ipv4_address(const uint8_t data[4])
    : m_octets{data[0], data[1], data[2], data[3]}
{}

ipv4_address::ipv4_address(uint32_t addr)
    : m_addr(htonl(addr))
{}

bool ipv4_address::is_loopback() const
{
    return (m_octets[0] == 127);
}

bool ipv4_address::is_multicast() const
{
    return (m_octets[0] >= 224 && m_octets[0] < 240);
}

uint32_t ipv4_address::data() const
{
    return (ntohl(m_addr));
}

uint8_t ipv4_address::operator[](size_t idx) const
{
    if (idx > 3) {
        throw std::out_of_range(std::to_string(idx) + " is not between 0 and 3");
    }
    return m_octets[idx];
}

std::string to_string(const ipv4_address& addr)
{
    char buffer[INET6_ADDRSTRLEN];
    auto ipv4 = htonl(addr.data());
    const char *p = inet_ntop(AF_INET, &ipv4, buffer, INET6_ADDRSTRLEN);
    return (std::string(p));
}

int compare(const ipv4_address& lhs, const ipv4_address& rhs)
{
    if      (lhs.data() < rhs.data()) return (-1);
    else if (lhs.data() > rhs.data()) return (1);
    else return (0);
}

}
}
