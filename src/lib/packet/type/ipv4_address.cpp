#include <stdexcept>

#include <arpa/inet.h>

#include "packet/type/ipv4_address.hpp"

namespace libpacket::type {

static void set_octets_from_string(ipv4_address& ipv4, const std::string& input)
{
    if (inet_pton(AF_INET, input.c_str(), ipv4.octets.data()) == 0) {
        throw std::runtime_error("Invalid IPv4 address: " + input);
    }
}

ipv4_address::ipv4_address(const char* input)
{
    set_octets_from_string(*this, std::string(input));
}

ipv4_address::ipv4_address(const std::string& input)
{
    set_octets_from_string(*this, input);
}

ipv4_address::ipv4_address(std::initializer_list<uint8_t> data)
{
    if (data.size() != 4) { throw std::runtime_error("4 items required"); }
    store(data);
}

ipv4_address::ipv4_address(const uint8_t data[4]) noexcept
{
    store({data[0], data[1], data[2], data[3]});
}

ipv4_address::ipv4_address(uint32_t addr) noexcept { store(addr); }

std::string to_string(const ipv4_address& addr)
{
    auto buffer = std::array<char, INET_ADDRSTRLEN>{};
    const char* p = inet_ntop(AF_INET,
                              reinterpret_cast<const void*>(addr.data()),
                              buffer.data(),
                              buffer.size());
    return (std::string(p));
}

bool is_loopback(const ipv4_address& addr) { return (addr.octets[0] == 127); }

bool is_multicast(const ipv4_address& addr)
{
    return (addr.octets[0] >= 224 && addr.octets[0] < 240);
}

} // namespace libpacket::type
