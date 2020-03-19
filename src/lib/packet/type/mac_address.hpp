#ifndef _LIB_PACKET_TYPE_MAC_ADDRESS_HPP_
#define _LIB_PACKET_TYPE_MAC_ADDRESS_HPP_

#include "packet/type/endian.hpp"

namespace packet::type {

class mac_address final : public endian::field<6>
{
public:
    mac_address();
    mac_address(std::string_view);

    template <typename T>
    mac_address(
        typename std::enable_if_t<!std::is_same_v<char*, T>, const T&> data)
    {
        store(data);
    }

    template <typename T>
    mac_address(typename std::enable_if_t<!std::is_same_v<char*, T>, T&&> data)
    {
        store(std::forward<T>(data));
    }

    bool is_unicast() const;
    bool is_multicast() const;
    bool is_broadcast() const;
    bool is_globally_unique() const;
    bool is_local_admin() const;

    static constexpr uint8_t group_addr_bit = (1 << 0);
    static constexpr uint8_t local_admin_bit = (1 << 1);
};

std::string to_string(const mac_address&);

} // namespace packet::type

namespace std {

template <> struct hash<packet::type::mac_address>
{
    size_t operator()(const packet::type::mac_address& mac) const
    {
        return (std::hash<uint64_t>{}(mac.load<uint64_t>()));
    }
};

} // namespace std

#endif /* _LIB_PACKET_TYPE_MAC_ADDRESS_HPP_ */
