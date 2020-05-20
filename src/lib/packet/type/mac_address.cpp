#include <stdexcept>

#include "packet/type/mac_address.hpp"

namespace libpacket::type {

mac_address::mac_address(std::string_view input)
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
            throw std::runtime_error("Too many octets in MAC address "
                                     + std::string(input));
        }

        pos = input.find_first_of(delimiters, beg + 1);
        auto value =
            std::strtol(input.substr(beg, pos - beg).data(), nullptr, 16);
        if (value < 0 || 0xff < value) {
            throw std::runtime_error("MAC address octet "
                                     + std::string(input.substr(beg, pos - beg))
                                     + " is not between 0x00 and 0xff");
        }

        octets[idx++] = value;
    }
    if (idx != 6) {
        throw std::runtime_error("Too few octets in MAC address "
                                 + std::string(input));
    }
}

mac_address::mac_address(std::initializer_list<uint8_t> data)
    : endian::field<6>(data)
{
    if (data.size() != 6) { throw std::runtime_error("6 items required"); }
}

mac_address::mac_address(const uint8_t data[6]) noexcept
{
    store({data[0], data[1], data[2], data[3], data[4], data[5]});
}

mac_address::mac_address(uint64_t addr) noexcept { store(addr); }

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

static constexpr uint8_t group_addr_bit = (1 << 0);
static constexpr uint8_t local_admin_bit = (1 << 1);

bool is_unicast(const mac_address& mac)
{
    return ((mac.octets[0] & group_addr_bit) == 0);
}

bool is_multicast(const mac_address& mac)
{
    return ((mac.octets[0] & group_addr_bit) == group_addr_bit);
}

bool is_broadcast(const mac_address& mac)
{
    return (mac.octets[0] == 0xff && mac.octets[1] == 0xff
            && mac.octets[2] == 0xff && mac.octets[3] == 0xff
            && mac.octets[4] == 0xff && mac.octets[5] == 0xff);
}

bool is_globally_unique(const mac_address& mac)
{
    return ((mac.octets[0] & local_admin_bit) == 0);
}

bool is_local_admin(const mac_address& mac)
{
    return ((mac.octets[0] & local_admin_bit) == local_admin_bit);
}

} // namespace libpacket::type
