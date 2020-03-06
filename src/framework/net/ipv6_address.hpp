#ifndef _OP_FRAMEWORK_NET_IPV6_ADDRESS_HPP_
#define _OP_FRAMEWORK_NET_IPV6_ADDRESS_HPP_

#include <cstdint>
#include <string>
#include <array>

namespace openperf {
namespace net {

/**
 * Immutable IPv6 Address
 */
class ipv6_address
{
public:
    static constexpr size_t SIZE = 16;
    static constexpr size_t SIZE_IN_BITS = SIZE * 8;
    static constexpr size_t SIZE_IN_U16 = SIZE / sizeof(uint16_t);
    static constexpr size_t SIZE_IN_U32 = SIZE / sizeof(uint32_t);
    static constexpr size_t SIZE_IN_U64 = SIZE / sizeof(uint64_t);

    typedef std::array<uint8_t, SIZE> data_u8_t;

    ipv6_address();
    ipv6_address(const std::string& str);
    ipv6_address(std::initializer_list<uint8_t> data);
    ipv6_address(data_u8_t data);
    ipv6_address(const uint8_t data[SIZE]);

    bool is_loopback() const;
    bool is_multicast() const;
    bool is_linklocal() const;

    const data_u8_t& data() const { return m_data; }
    uint8_t operator[](size_t idx) const;

    void to_host_array(uint16_t* data) const;
    void to_host_array(uint32_t* data) const;
    void to_host_array(uint64_t* data) const;

    void to_net_array(uint16_t* data) const;
    void to_net_array(uint32_t* data) const;
    void to_net_array(uint64_t* data) const;

    uint16_t get_host_u16(size_t idx) const;
    uint32_t get_host_u32(size_t idx) const;
    uint64_t get_host_u64(size_t idx) const;

    /**
     * Create an IPv6 address mask from the specified prefix length.
     * @param[in] prefix_length
     *   Prefix length in bits
     * @return
     *   IPv6 adddress mask
     */
    static ipv6_address make_prefix_mask(uint8_t prefix_length);

    friend ipv6_address operator&(const ipv6_address& lhs,
                                  const ipv6_address& rhs);

private:
    data_u8_t m_data;

    void set_host_u16(size_t idx, uint16_t val);
    void set_host_u32(size_t idx, uint32_t val);
    void set_host_u64(size_t idx, uint64_t val);
};

std::string to_string(const ipv6_address&);

int compare(const ipv6_address&, const ipv6_address&);

inline bool operator==(const ipv6_address& lhs, const ipv6_address& rhs)
{
    return compare(lhs, rhs) == 0;
}
inline bool operator!=(const ipv6_address& lhs, const ipv6_address& rhs)
{
    return compare(lhs, rhs) != 0;
}
inline bool operator<(const ipv6_address& lhs, const ipv6_address& rhs)
{
    return compare(lhs, rhs) < 0;
}
inline bool operator>(const ipv6_address& lhs, const ipv6_address& rhs)
{
    return compare(lhs, rhs) > 0;
}
inline bool operator<=(const ipv6_address& lhs, const ipv6_address& rhs)
{
    return compare(lhs, rhs) <= 0;
}
inline bool operator>=(const ipv6_address& lhs, const ipv6_address& rhs)
{
    return compare(lhs, rhs) >= 0;
}

inline std::ostream& operator<<(std::ostream& os, const ipv6_address& value)
{
    os << to_string(value);
    return os;
}

} // namespace net
} // namespace openperf

#endif /* _OP_FRAMEWORK_NET_IPV6_ADDRESS_HPP_ */
