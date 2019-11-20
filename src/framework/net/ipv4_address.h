#ifndef _OP_FRAMEWORK_NET_IPV4_ADDRESS_H_
#define _OP_FRAMEWORK_NET_IPV4_ADDRESS_H_

#include <cstdint>
#include <string>

namespace openperf {
namespace net {

class ipv4_address
{
public:
    ipv4_address();
    ipv4_address(const std::string&);
    ipv4_address(std::initializer_list<uint8_t>);
    ipv4_address(const uint8_t[4]);
    ipv4_address(uint32_t);

    bool is_loopback() const;
    bool is_multicast() const;

    uint32_t data() const;
    uint8_t operator[](size_t idx) const;

private:
    union {
        uint8_t  m_octets[4];
        uint32_t m_addr;
    };
};

std::string to_string(const ipv4_address&);

int compare(const ipv4_address&, const ipv4_address&);

inline bool operator==(const ipv4_address& lhs, const ipv4_address& rhs) { return compare(lhs, rhs) == 0; }
inline bool operator!=(const ipv4_address& lhs, const ipv4_address& rhs) { return compare(lhs, rhs) != 0; }
inline bool operator< (const ipv4_address& lhs, const ipv4_address& rhs) { return compare(lhs, rhs) <  0; }
inline bool operator> (const ipv4_address& lhs, const ipv4_address& rhs) { return compare(lhs, rhs) >  0; }
inline bool operator<=(const ipv4_address& lhs, const ipv4_address& rhs) { return compare(lhs, rhs) <= 0; }
inline bool operator>=(const ipv4_address& lhs, const ipv4_address& rhs) { return compare(lhs, rhs) >= 0; }

}
}

#endif /* _OP_FRAMEWORK_NET_IPV4_ADDRESS_H_ */
