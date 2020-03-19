#include <stdexcept>

#include <arpa/inet.h>

#include "packet/type/ipv4_address.hpp"

namespace packet::type {

ipv4_address::ipv4_address()
    : endian::field<4>()
{}

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

bool ipv4_address::is_loopback() const { return (octets[0] == 127); }

bool ipv4_address::is_multicast() const
{
    return (octets[0] >= 224 && octets[0] < 240);
}

std::string to_string(const ipv4_address& addr)
{
    auto buffer = std::array<char, INET_ADDRSTRLEN>{};
    const char* p = inet_ntop(AF_INET,
                              reinterpret_cast<const void*>(addr.data()),
                              buffer.data(),
                              buffer.size());
    return (std::string(p));
}

} // namespace packet::type
