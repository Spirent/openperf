#include <arpa/inet.h>

#include "packet/type/ipv6_address.hpp"

namespace packet::type {

ipv6_address::ipv6_address()
    : endian::field<16>()
{}

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

std::string to_string(const ipv6_address& addr)
{
    auto buffer = std::array<char, INET6_ADDRSTRLEN>{};
    const char* p = inet_ntop(AF_INET6,
                              reinterpret_cast<const void*>(addr.data()),
                              buffer.data(),
                              buffer.size());
    return (std::string(p));
}

} // namespace packet::type
