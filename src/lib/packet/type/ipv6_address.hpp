#ifndef _LIB_PACKET_TYPE_IPV6_ADDRESS_HPP_
#define _LIB_PACKET_TYPE_IPV6_ADDRESS_HPP_

#include "packet/type/endian.hpp"

namespace packet::type {

class ipv6_address final : public endian::field<16>
{
public:
    ipv6_address();
    ipv6_address(const char*);
    ipv6_address(const std::string&);

    template <typename T>
    ipv6_address(
        typename std::enable_if_t<!std::is_same_v<char*, T>, const T&> data)
    {
        store(data);
    }

    template <typename T>
    ipv6_address(typename std::enable_if_t<!std::is_same_v<char*, T>, T&&> data)
    {
        store(std::forward<T>(data));
    }
};

std::string to_string(const ipv6_address&);

} // namespace packet::type

#endif /* _LIB_PACKET_TYPE_IPV6_ADDRESS_HPP_ */
