#ifndef _ICP_FRAMEWORK_NET_MAC_ADDRESS_H_
#define _ICP_FRAMEWORK_NET_MAC_ADDRESS_H_

#include <cstdint>
#include <string>

namespace icp {
namespace net {

class mac_address
{
public:
    mac_address();
    mac_address(const std::string&);
    mac_address(std::initializer_list<uint8_t>);
    mac_address(const uint8_t[6]);

    bool is_unicast() const;
    bool is_multicast() const;
    bool is_broadcast() const;
    bool is_globally_unique() const;
    bool is_local_admin() const;

    uint8_t operator[](size_t idx) const;
    const uint8_t* data() const;

    friend int compare(const mac_address&, const mac_address&);

private:
    uint8_t m_octets[6];

    static constexpr uint8_t group_addr_bit  = (1 << 0);
    static constexpr uint8_t local_admin_bit = (1 << 1);
};

std::string to_string(const mac_address&);

inline bool operator==(const mac_address& lhs, const mac_address& rhs) { return compare(lhs, rhs) == 0; }
inline bool operator!=(const mac_address& lhs, const mac_address& rhs) { return compare(lhs, rhs) != 0; }
inline bool operator< (const mac_address& lhs, const mac_address& rhs) { return compare(lhs, rhs) <  0; }
inline bool operator> (const mac_address& lhs, const mac_address& rhs) { return compare(lhs, rhs) >  0; }
inline bool operator<=(const mac_address& lhs, const mac_address& rhs) { return compare(lhs, rhs) <= 0; }
inline bool operator>=(const mac_address& lhs, const mac_address& rhs) { return compare(lhs, rhs) >= 0; }

}
}

namespace std {

template <>
struct hash<icp::net::mac_address>
{
    size_t operator()(const icp::net::mac_address& mac) const noexcept
    {
        auto value = (static_cast<uint64_t>(mac[0]) << 40
                      | static_cast<uint64_t>(mac[1]) << 32
                      | mac[2] << 24
                      | mac[3] << 16
                      | mac[4] << 8
                      | mac[5]);
        return (std::hash<uint64_t>{}(value));
    }
};

}
#endif /* _ICP_FRAMEWORK_NET_MAC_ADDRESS_H_ */
