#include <arpa/inet.h>

#include "packet/type/ipv6_address.hpp"

namespace libpacket::type {

static void set_octets_from_string(ipv6_address& ipv6, const std::string& input)
{
    if (inet_pton(AF_INET6, input.c_str(), ipv6.octets.data()) == 0) {
        throw std::runtime_error("Invalid IPv6 address: " + input);
    }
}

ipv6_address::ipv6_address(const char* input)
{
    set_octets_from_string(*this, std::string(input));
}

ipv6_address::ipv6_address(const std::string& input)
{
    set_octets_from_string(*this, input);
}

ipv6_address::ipv6_address(std::initializer_list<uint8_t> data)
    : endian::field<16>(data)
{
    if (data.size() != 16) { throw std::runtime_error("16 items required"); }
}

ipv6_address::ipv6_address(const uint8_t data[16]) noexcept
{
    store({data[0],
           data[1],
           data[2],
           data[3],
           data[4],
           data[5],
           data[6],
           data[7],
           data[8],
           data[9],
           data[10],
           data[11],
           data[12],
           data[13],
           data[14],
           data[15]});
}

ipv6_address::ipv6_address(const uint32_t data[4]) noexcept
{
    store({data[0], data[1], data[2], data[3]});
}

ipv6_address::ipv6_address(uint64_t value) noexcept
{
    store({uint64_t{0}, value});
}

ipv6_address::ipv6_address(std::pair<uint64_t, uint64_t> value) noexcept
{
    store({value.first, value.second});
}

std::string to_string(const ipv6_address& addr)
{
    auto buffer = std::array<char, INET6_ADDRSTRLEN>{};
    const char* p = inet_ntop(AF_INET6,
                              reinterpret_cast<const void*>(addr.data()),
                              buffer.data(),
                              buffer.size());
    return (std::string(p));
}

bool is_loopback(const ipv6_address& addr)
{
    return (addr.octets[0] == 0 && addr.octets[1] == 0 && addr.octets[2] == 0
            && addr.octets[3] == 0 && addr.octets[4] == 0 && addr.octets[5] == 0
            && addr.octets[6] == 0 && addr.octets[7] == 0 && addr.octets[8] == 0
            && addr.octets[9] == 0 && addr.octets[10] == 0
            && addr.octets[11] == 0 && addr.octets[12] == 0
            && addr.octets[13] == 0 && addr.octets[14] == 0
            && addr.octets[15] == 1);
}

bool is_multicast(const ipv6_address& addr) { return (addr.octets[0] == 0xff); }

bool is_linklocal(const ipv6_address& addr)
{
    return (addr.octets[0] == 0xfe && (addr.octets[1] & 0xc0) == 0x80);
}

} // namespace libpacket::type
