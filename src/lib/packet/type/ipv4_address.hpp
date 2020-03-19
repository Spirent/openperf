#ifndef _LIB_PACKET_TYPE_IPV4_ADDRESS_HPP_
#define _LIB_PACKET_TYPE_IPV4_ADDRESS_HPP_

#include "packet/type/endian.hpp"

namespace packet::type {

class ipv4_address final : public endian::field<4>
{
public:
    ipv4_address();
    ipv4_address(const char*);
    ipv4_address(const std::string&);

    template <typename T>
    ipv4_address(
        typename std::enable_if_t<!std::is_same_v<char*, T>, const T&> data)
    {
        store(data);
    }

    template <typename T>
    ipv4_address(typename std::enable_if_t<!std::is_same_v<char*, T>, T&&> data)
    {
        store(std::forward<T>(data));
    }

    bool is_loopback() const;
    bool is_multicast() const;
};

std::string to_string(const ipv4_address&);

} // namespace packet::type

#endif /* _LIB_PACKET_TYPE_IPV4_ADDRESS_HPP_ */
