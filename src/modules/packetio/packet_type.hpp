#ifndef _OP_PACKETIO_PACKET_TYPE_HPP_
#define _OP_PACKETIO_PACKET_TYPE_HPP_

#include <cstdint>
#include <type_traits>

namespace openperf::packetio::packet::packet_type {

enum class ethernet : uint32_t {
    none = 0x0,
    ether = 0x1,
    timesync = 0x2,
    arp = 0x3,
    lldp = 0x4,
    nsh = 0x5,
    vlan = 0x6,
    qinq = 0x7,
    pppoe = 0x8,
    fcoe = 0x9,
    mpls = 0xa,
    mask = 0xf
};

enum class ip : uint32_t {
    none = 0,
    ipv4 = 0x10,
    ipv4_ext = 0x30,
    ipv6 = 0x40,
    ipv4_ext_unknown = 0x90,
    ipv6_ext = 0xc0,
    ipv6_ext_unknown = 0xe0,
    mask = 0xf0
};

enum class protocol : uint32_t {
    none = 0,
    tcp = 0x100,
    udp = 0x200,
    fragment = 0x300,
    sctp = 0x400,
    icmp = 0x500,
    non_fragment = 0x600,
    igmp = 0x700,
    mask = 0xf00
};

enum class tunnel : uint32_t {
    none = 0,
    ip = 0x1000,
    gre = 0x2000,
    vxlan = 0x3000,
    nvgre = 0x4000,
    geneve = 0x5000,
    grenat = 0x6000,
    gtpc = 0x7000,
    gtpu = 0x8000,
    esp = 0x9000,
    l2tp = 0xa000,
    vxlan_gpe = 0xb000,
    mpls_in_gre = 0xc000,
    mpls_in_udp = 0xd000,
    mask = 0xf000
};

enum class inner_ethernet : uint32_t {
    none = 0,
    ether = 0x10000,
    vlan = 0x20000,
    qinq = 0x30000,
    mask = 0xf0000
};

enum class inner_ip : uint32_t {
    none = 0,
    ipv4 = 0x100000,
    ipv4_ext = 0x200000,
    ipv6 = 0x300000,
    ipv4_ext_unknown = 0x400000,
    ipv6_ext = 0x500000,
    ipv6_ext_unknown = 0x600000,
    mask = 0xf00000
};

enum class inner_protocol : uint32_t {
    none = 0,
    tcp = 0x1000000,
    udp = 0x2000000,
    fragment = 0x3000000,
    sctp = 0x4000000,
    icmp = 0x5000000,
    non_fragment = 0x6000000,
    mask = 0xf000000
};

template <typename Enum> struct is_packet_type_flag : std::false_type
{};
template <> struct is_packet_type_flag<ethernet> : std::true_type
{};
template <> struct is_packet_type_flag<ip> : std::true_type
{};
template <> struct is_packet_type_flag<protocol> : std::true_type
{};
template <> struct is_packet_type_flag<tunnel> : std::true_type
{};
template <> struct is_packet_type_flag<inner_ethernet> : std::true_type
{};
template <> struct is_packet_type_flag<inner_ip> : std::true_type
{};
template <> struct is_packet_type_flag<inner_protocol> : std::true_type
{};

template <typename T>
inline constexpr bool is_packet_type_flag_v = is_packet_type_flag<T>::value;

struct flags
{
    uint32_t value;

    template <typename Enum,
              typename = typename std::enable_if_t<is_packet_type_flag_v<Enum>>>
    constexpr flags(const Enum value)
        : value(static_cast<uint32_t>(value))
    {}

    constexpr flags(uint32_t value)
        : value(value)
    {}

    flags() {}

    constexpr explicit operator bool() const { return (value != 0); }

    constexpr bool operator==(const flags& other) const
    {
        return (value == other.value);
    }
};

/***
 * & operators
 ***/

template <typename Enum1, typename Enum2>
constexpr typename std::enable_if_t<
    std::conjunction_v<is_packet_type_flag<Enum1>, is_packet_type_flag<Enum2>>,
    flags>
operator&(const Enum1& lhs, const Enum2& rhs)
{
    using enum_type = std::common_type_t<std::underlying_type_t<Enum1>,
                                         std::underlying_type_t<Enum2>>;
    return (flags(static_cast<enum_type>(lhs) & static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<is_packet_type_flag_v<Enum>, flags>
operator&(const Enum& lhs, const flags& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (flags(static_cast<enum_type>(lhs) & rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<is_packet_type_flag_v<Enum>, flags>
operator&(const flags& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (flags(lhs.value & static_cast<enum_type>(rhs)));
}

constexpr flags operator&(const flags& lhs, const flags& rhs)
{
    return (flags(lhs.value & rhs.value));
}

/***
 * | operators
 ***/

template <typename Enum1, typename Enum2>
constexpr typename std::enable_if_t<
    std::conjunction_v<is_packet_type_flag<Enum1>, is_packet_type_flag<Enum2>>,
    flags>
operator|(const Enum1& lhs, const Enum2& rhs)
{
    using enum_type = std::common_type_t<std::underlying_type_t<Enum1>,
                                         std::underlying_type_t<Enum2>>;
    return (flags(static_cast<enum_type>(lhs) | static_cast<enum_type>(rhs)));
}

template <typename Enum>
constexpr typename std::enable_if_t<is_packet_type_flag_v<Enum>, flags>
operator|(const Enum& lhs, const flags& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (flags(static_cast<enum_type>(lhs) | rhs.value));
}

template <typename Enum>
constexpr typename std::enable_if_t<is_packet_type_flag_v<Enum>, flags>
operator|(const flags& lhs, const Enum& rhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (flags(lhs.value | static_cast<enum_type>(rhs)));
}

constexpr flags operator|(const flags& lhs, const flags& rhs)
{
    return (flags(lhs.value | rhs.value));
}

/***
 * ~ operators
 ***/

template <typename Enum>
constexpr std::enable_if_t<is_packet_type_flag_v<Enum>, flags>
operator~(const Enum& lhs)
{
    using enum_type = std::underlying_type_t<Enum>;
    return (flags(~(static_cast<enum_type>(lhs))));
}

constexpr flags operator~(const flags& lhs) { return (flags(~lhs.value)); }

} // namespace openperf::packetio::packet::packet_type

#endif /* _OP_PACKETIO_PACKET_TYPE_HPP_ */
