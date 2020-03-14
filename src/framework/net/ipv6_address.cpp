#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <algorithm>

#include "net/ipv6_address.hpp"
#include "net/byte_swap.hpp"

namespace openperf::net {

ipv6_address::ipv6_address()
    : m_data{}
{}

ipv6_address::ipv6_address(const std::string& str)
{
    if (inet_pton(AF_INET6, str.c_str(), m_data.data()) == 0) {
        throw std::runtime_error("Invalid IPv6 address: " + str);
    }
}

ipv6_address::ipv6_address(std::initializer_list<uint8_t> data)
{
    if (data.size() != SIZE) {
        throw std::runtime_error(std::to_string(SIZE) + " items required");
    }
    std::copy(data.begin(), data.end(), m_data.begin());
}

ipv6_address::ipv6_address(const uint8_t data[SIZE])
{
    std::copy_n(data, SIZE, m_data.begin());
}

ipv6_address::ipv6_address(ipv6_address::data_u8_t data)
    : m_data(data)
{}

bool ipv6_address::is_loopback() const
{
    return (m_data[0] == 0 && m_data[1] == 0 && m_data[2] == 0 && m_data[3] == 0
            && m_data[4] == 0 && m_data[5] == 0 && m_data[6] == 0
            && m_data[7] == 0 && m_data[8] == 0 && m_data[9] == 0
            && m_data[10] == 0 && m_data[11] == 0 && m_data[12] == 0
            && m_data[13] == 0 && m_data[14] == 0 && m_data[15] == 1);
}

bool ipv6_address::is_multicast() const { return (m_data[0] == 0xff); }

bool ipv6_address::is_linklocal() const
{
    return (m_data[0] == 0xfe && (m_data[1] & 0xc0) == 0x80);
}

uint8_t ipv6_address::operator[](size_t idx) const
{
    if (idx > (SIZE - 1)) {
        throw std::out_of_range(std::to_string(idx) + " is not between 0 and "
                                + std::to_string(SIZE - 1));
    }
    return m_data[idx];
}

void ipv6_address::to_host_array(uint16_t data[SIZE_IN_U16]) const
{
    for (size_t i = 0; i < SIZE_IN_U16; ++i) { data[i] = get_host_u16(i); }
}

void ipv6_address::to_host_array(uint32_t data[SIZE_IN_U32]) const
{
    for (size_t i = 0; i < SIZE_IN_U32; ++i) { data[i] = get_host_u32(i); }
}

void ipv6_address::to_host_array(uint64_t data[SIZE_IN_U64]) const
{
    for (size_t i = 0; i < SIZE_IN_U64; ++i) { data[i] = get_host_u64(i); }
}

void ipv6_address::to_net_array(uint16_t data[SIZE_IN_U16]) const
{
    std::memcpy(data, m_data.data(), SIZE);
}

void ipv6_address::to_net_array(uint32_t data[SIZE_IN_U32]) const
{
    std::memcpy(data, m_data.data(), SIZE);
}

void ipv6_address::to_net_array(uint64_t data[SIZE_IN_U64]) const
{
    std::memcpy(data, m_data.data(), SIZE);
}

uint16_t ipv6_address::get_host_u16(size_t idx) const
{
    if (idx > (SIZE_IN_U16 - 1)) {
        throw std::out_of_range(std::to_string(idx) + " is not between 0 and "
                                + std::to_string(SIZE_IN_U16 - 1));
    }
    auto ptr = reinterpret_cast<const uint16_t*>(m_data.data()
                                                 + idx * sizeof(uint16_t));
    return ntohs(*ptr);
}

void ipv6_address::set_host_u16(size_t idx, uint16_t val)
{
    if (idx > (SIZE_IN_U16 - 1)) {
        throw std::out_of_range(std::to_string(idx) + " is not between 0 and "
                                + std::to_string(SIZE_IN_U16 - 1));
    }
    auto ptr =
        reinterpret_cast<uint16_t*>(m_data.data() + idx * sizeof(uint16_t));
    *ptr = htons(val);
}

uint32_t ipv6_address::get_host_u32(size_t idx) const
{
    if (idx > (SIZE_IN_U32 - 1)) {
        throw std::out_of_range(std::to_string(idx) + " is not between 0 and "
                                + std::to_string(SIZE_IN_U32 - 1));
    }
    auto ptr = reinterpret_cast<const uint32_t*>(m_data.data()
                                                 + idx * sizeof(uint32_t));
    return ntohl(*ptr);
}

void ipv6_address::set_host_u32(size_t idx, uint32_t val)
{
    if (idx > (SIZE_IN_U32 - 1)) {
        throw std::out_of_range(std::to_string(idx) + " is not between 0 and "
                                + std::to_string(SIZE_IN_U32 - 1));
    }
    auto ptr =
        reinterpret_cast<uint32_t*>(m_data.data() + idx * sizeof(uint32_t));
    *ptr = htonl(val);
}

uint64_t ipv6_address::get_host_u64(size_t idx) const
{
    if (idx > (SIZE_IN_U64 - 1)) {
        throw std::out_of_range(std::to_string(idx) + " is not between 0 and "
                                + std::to_string(SIZE_IN_U64 - 1));
    }
    auto ptr = reinterpret_cast<const uint64_t*>(m_data.data()
                                                 + idx * sizeof(uint64_t));
    return ntohll(*ptr);
}

void ipv6_address::set_host_u64(size_t idx, uint64_t val)
{
    if (idx > (SIZE_IN_U64 - 1)) {
        throw std::out_of_range(std::to_string(idx) + " is not between 0 and "
                                + std::to_string(SIZE_IN_U64 - 1));
    }
    auto ptr =
        reinterpret_cast<uint64_t*>(m_data.data() + idx * sizeof(uint64_t));
    *ptr = htonll(val);
}

std::string to_string(const ipv6_address& addr)
{
    char buffer[INET6_ADDRSTRLEN];
    auto data = addr.data();
    const char* p = inet_ntop(AF_INET6, data.data(), buffer, INET6_ADDRSTRLEN);
    return (std::string(p));
}

int compare(const ipv6_address& lhs, const ipv6_address& rhs)
{
    if (lhs.data() < rhs.data())
        return (-1);
    else if (lhs.data() > rhs.data())
        return (1);
    else
        return (0);
}

ipv6_address operator&(const ipv6_address& lhs, const ipv6_address& rhs)
{
    ipv6_address result;
    std::transform(lhs.m_data.begin(),
                   lhs.m_data.end(),
                   rhs.m_data.begin(),
                   result.m_data.begin(),
                   [](uint8_t lv, uint8_t rv) { return (lv & rv); });
    return result;
}

ipv6_address ipv6_address::make_prefix_mask(uint8_t prefix_length)
{
    if (prefix_length > SIZE_IN_BITS) {
        throw std::out_of_range(std::to_string(prefix_length)
                                + " is larger than "
                                + std::to_string(SIZE_IN_BITS));
    }

    size_t bytes = prefix_length / 8;
    size_t bits_rem = prefix_length % 8;
    size_t i;
    ipv6_address addr;

    for (i = 0; i < bytes; ++i) { addr.m_data[i] = 0xff; }
    if (bits_rem) { addr.m_data[i] = 0xff << (8 - bits_rem); }
    return addr;
}

} // namespace openperf::net
