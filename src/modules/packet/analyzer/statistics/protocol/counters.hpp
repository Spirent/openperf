#ifndef _OP_ANALYZER_STATISTICS_PROTOCOL_COUNTERS_HPP_
#define _OP_ANALYZER_STATISTICS_PROTOCOL_COUNTERS_HPP_

#include <array>

#include "packet/analyzer/statistics/common.hpp"
#include "spirent_pga/api.h"

namespace openperf::packet::analyzer::statistics::protocol {

/*
 * These packet_type_counters are designed to provide accumulators for DPDK's
 * packet type field.
 *
 * The packet type field is a 32 bit field with sub-fields containing values
 * indicating the protocol at the different layers of the packet.  For example,
 * an Ethernet/IPv4/UDP packet would have the packet type value 0x211 while
 * an Ethernet/IPv6/ICMP packet would have the value 0x541.
 *
 * The sub-fields are as follows:
 * - bits 31:28 are currently unused
 * - bits 27:24 contain the inner layer 4 type
 * - bits 23:20 contain the inner layer 3 type
 * - bits 19:16 contain the inner layer 2 type
 * - bits 15:12 contain the tunnel type
 * - bits 11:8 contain the layer 4 type (or outer type if tunneled)
 * - bits 7:4 contain the layer 3 type (or outer type if tunneled)
 * - bits 3:0 contain the layer 2 type (or outer type if tunneled)
 *
 * Each derived type contains a mask indicating which bits are specific to that
 * type and an enum that maps the counter name to an index.  Clients access
 * individual counter values by using the index enum, e.g.
 * object[object::index::enum].
 *
 * Note: some packet type index values are unused and that is reflected in both
 * the mask and the enum values.
 */

template <size_t Size> struct packet_type_counters
{
    static_assert((Size & (Size - 1)) == 0); /* must be power of two */

    using array = std::array<stat_t, Size>;
    array counters = {};

    constexpr typename array::reference
    operator[](typename array::size_type idx)
    {
        return (counters[idx]);
    }

    constexpr typename array::const_reference
    operator[](typename array::size_type idx) const
    {
        return (counters[idx]);
    }

    template <typename Enum>
    constexpr std::enable_if_t<std::is_enum_v<Enum>, typename array::reference>
    operator[](Enum e)
    {
        return (counters[static_cast<std::underlying_type_t<Enum>>(e)]);
    }

    template <typename Enum>
    constexpr std::enable_if_t<std::is_enum_v<Enum>,
                               typename array::const_reference>
    operator[](Enum e) const
    {
        return (counters[static_cast<std::underlying_type_t<Enum>>(e)]);
    }

    packet_type_counters& operator+=(const packet_type_counters& rhs)
    {
        std::transform(std::begin(counters),
                       std::end(counters),
                       std::begin(rhs.counters),
                       counters.data(),
                       [](const auto& x, const auto& y) { return (x + y); });
        return (*this);
    }

    friend packet_type_counters operator+(packet_type_counters lhs,
                                          const packet_type_counters& rhs)
    {
        lhs += rhs;
        return (lhs);
    }
};

struct ethernet final : public packet_type_counters<16>
{
    static constexpr uint32_t mask = 0xf;

    enum class index {
        none = 0,
        ether,
        timesync,
        arp,
        lldp,
        nsh,
        vlan,
        qinq,
        pppoe,
        fcoe,
        mpls,
        unused_b,
        unused_c,
        unused_d,
        unused_e,
    };

    ethernet() = default;
    ethernet(const packet_type_counters<16>& base)
        : packet_type_counters<16>(base)
    {}
};

struct ip final : packet_type_counters<16>
{
    static constexpr uint32_t mask = 0xf0;

    enum class index {
        none = 0,
        ipv4,
        unused_2,
        ipv4_ext,
        ipv6,
        unused_5,
        unused_6,
        unused_7,
        unused_8,
        ipv4_ext_unknown,
        unused_a,
        unused_b,
        ipv6_ext,
        unused_d,
        ipv6_ext_unknown
    };

    ip() = default;
    ip(const packet_type_counters<16>& base)
        : packet_type_counters<16>(base)
    {}
};

struct protocol final : packet_type_counters<8>
{
    static constexpr uint32_t mask = 0x700;

    enum class index {
        none = 0,
        tcp,
        udp,
        fragment,
        sctp,
        icmp,
        non_fragment,
        igmp,
    };

    protocol() = default;
    protocol(const packet_type_counters<8>& base)
        : packet_type_counters<8>(base)
    {}
};

struct tunnel final : packet_type_counters<16>
{
    static constexpr uint32_t mask = 0xf000;

    enum class index {
        none = 0,
        ip,
        gre,
        vxlan,
        nvgre,
        geneve,
        grenat,
        gtpc,
        gtpu,
        esp,
        l2tp,
        vxlan_gpe,
        mpls_in_gre,
        mpls_in_udp,
        unused_e,
    };

    tunnel() = default;
    tunnel(const packet_type_counters<16>& base)
        : packet_type_counters<16>(base)
    {}
};

struct inner_ethernet final : packet_type_counters<4>
{
    static constexpr uint32_t mask = 0x30000;

    enum class index {
        none = 0,
        ether,
        vlan,
        qinq,
    };

    inner_ethernet() = default;
    inner_ethernet(const packet_type_counters<4>& base)
        : packet_type_counters<4>(base)
    {}
};

struct inner_ip final : packet_type_counters<8>
{
    static constexpr uint32_t mask = 0x700000;

    enum class index {
        none = 0,
        ipv4,
        ipv4_ext,
        ipv6,
        ipv4_ext_unknown,
        ipv6_ext,
        ipv6_ext_unknown,
        unused_7,
    };

    inner_ip() = default;
    inner_ip(const packet_type_counters<8>& base)
        : packet_type_counters<8>(base)
    {}
};

struct inner_protocol final : packet_type_counters<8>
{
    static constexpr uint32_t mask = 0x7000000;

    enum class index {
        none = 0,
        tcp,
        udp,
        fragment,
        sctp,
        icmp,
        non_fragment,
        unused_7,
    };

    inner_protocol() = default;
    inner_protocol(const packet_type_counters<8>& base)
        : packet_type_counters<8>(base)
    {}
};

namespace impl {

template <template <class> class Predicate, class Tuple> struct filter;

template <template <class> class Predicate, class... Ts>
struct filter<Predicate, std::tuple<Ts...>>
{
    using type =
        decltype(std::tuple_cat(std::conditional_t<Predicate<Ts>::value,
                                                   std::tuple<Ts>,
                                                   std::tuple<>>{}...));
};

template <typename T> struct has_mask
{
    template <typename U, typename = std::void_t<>>
    struct has_mask_impl : std::false_type
    {};

    template <typename U>
    struct has_mask_impl<U, std::void_t<decltype(std::declval<U>().mask)>>
        : std::true_type
    {};

    static constexpr bool value = has_mask_impl<T>::value;
};

template <typename... T>
constexpr auto get_packet_type_mask_impl(const std::tuple<T...>&)
{
    using tuple_type = typename std::tuple_element<0, std::tuple<T...>>::type;
    using mask_type = typename std::remove_cv<decltype(
        std::declval<tuple_type>().mask)>::type;

    auto masks = std::array<mask_type, sizeof...(T)>{};
    auto idx = 0U;
    (void(masks[idx++] = T::mask), ...);
    return (masks);
}

template <typename... T>
constexpr auto get_packet_type_mask(const std::tuple<T...>&)
{
    using packet_type_tuple = typename filter<has_mask, std::tuple<T...>>::type;
    return (get_packet_type_mask_impl(packet_type_tuple{}));
}

template <typename StatsTuple, typename... T>
constexpr auto get_packet_type_counters_impl(StatsTuple& tuple,
                                             const std::tuple<T...>&)
{
    using tuple_type = typename std::tuple_element<0, std::tuple<T...>>::type;
    using counter_type = decltype(std::declval<tuple_type>().counters.data());

    auto counters = std::array<counter_type, sizeof...(T)>{};
    auto idx = 0U;
    ((counters[idx++] = std::get<T>(tuple).counters.data()), ...);
    return (counters);
}

template <typename... T>
constexpr auto get_packet_type_counters(std::tuple<T...>& tuple)
{
    using packet_type_tuple = typename filter<has_mask, std::tuple<T...>>::type;
    return (get_packet_type_counters_impl(tuple, packet_type_tuple{}));
}

} // namespace impl

/**
 * Update functions for stat structures
 **/

template <typename BasicStat>
inline void update(BasicStat& stat,
                   uint64_t nb_frames,
                   typename BasicStat::timestamp rx) noexcept
{
    if (!stat.first()) { stat.first_ = rx; }
    stat.frames += nb_frames;
    stat.last_ = rx;
}

template <typename StatsTuple>
void update(StatsTuple& tuple, const uint32_t fields[], uint16_t nb_fields)
{
    /*
     * Extract the appropriate mask for the tuple type and sum up the
     * field data.
     */
    if constexpr (std::tuple_size_v<StatsTuple> != 0) {
        static constexpr auto masks = impl::get_packet_type_mask(StatsTuple{});
        auto counters = impl::get_packet_type_counters(tuple);

        pga_unpack_and_sum_indexicals(
            fields, nb_fields, masks.data(), masks.size(), counters.data());
    }
}

/**
 * Debug methods
 **/

void dump(std::ostream& os, const ethernet& stat);
void dump(std::ostream& os, const ip& stat);
void dump(std::ostream& os, const protocol& stat);
void dump(std::ostream& os, const tunnel& stat);
void dump(std::ostream& os, const inner_ethernet& stat);
void dump(std::ostream& os, const inner_ip& stat);
void dump(std::ostream& os, const inner_protocol& stat);

template <typename StatsTuple>
void dump(std::ostream& os, const StatsTuple& tuple)
{
    os << __PRETTY_FUNCTION__ << std::endl;

    if constexpr (has_type<ethernet, StatsTuple>::value) {
        dump(os, get_counter<ethernet, StatsTuple>(tuple));
    }

    if constexpr (has_type<ip, StatsTuple>::value) {
        dump(os, get_counter<ip, StatsTuple>(tuple));
    }

    if constexpr (has_type<protocol, StatsTuple>::value) {
        dump(os, get_counter<protocol, StatsTuple>(tuple));
    }

    if constexpr (has_type<tunnel, StatsTuple>::value) {
        dump(os, get_counter<tunnel, StatsTuple>(tuple));
    }

    if constexpr (has_type<inner_ethernet, StatsTuple>::value) {
        dump(os, get_counter<inner_ethernet, StatsTuple>(tuple));
    }

    if constexpr (has_type<inner_ip, StatsTuple>::value) {
        dump(os, get_counter<inner_ip, StatsTuple>(tuple));
    }

    if constexpr (has_type<inner_protocol, StatsTuple>::value) {
        dump(os, get_counter<inner_protocol, StatsTuple>(tuple));
    }
}

} // namespace openperf::packet::analyzer::statistics::protocol

#endif /* _OP_ANALYZER_STATISTICS_PROTOCOL_COUNTERS_HPP_ */
