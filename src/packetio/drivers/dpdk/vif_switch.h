#ifndef _ICP_PACKETIO_DPDK_SWITCH_H_
#define _ICP_PACKETIO_DPDK_SWITCH_H_

#include <optional>
#include <unordered_map>
#include <vector>

#include "net/net_types.h"

namespace icp {
namespace packetio {
namespace dpdk {

template <typename Interface>
class vif_switch {
    std::unordered_map<uint64_t, Interface> m_interfaces;
    std::vector<std::vector<Interface>> m_port_interfaces;

    static uint64_t __attribute__((const)) to_key(uint16_t id, const net::mac_address& mac)
    {
        return (static_cast<uint64_t>(id) << 48
                | static_cast<uint64_t>(mac[0]) << 40
                | static_cast<uint64_t>(mac[1]) << 32
                | mac[2] << 24
                | mac[3] << 16
                | mac[4] << 8
                | mac[5]);
    }

    static uint64_t __attribute__((const)) to_key(uint16_t id, const uint8_t octets[6])
    {
        return (static_cast<uint64_t>(id) << 48
                | static_cast<uint64_t>(octets[0]) << 40
                | static_cast<uint64_t>(octets[1]) << 32
                | octets[2] << 24
                | octets[3] << 16
                | octets[4] << 8
                | octets[5]);
    }

public:
    void insert(uint16_t id, const net::mac_address& mac, const Interface& intf)
    {
        while (m_port_interfaces.size() < id + 1) {
            m_port_interfaces.emplace_back(std::vector<Interface>());
        }

        m_interfaces[to_key(id, mac)] = intf;
        m_port_interfaces[id].push_back(intf);
    }

    void erase(uint16_t id, const net::mac_address& mac, const Interface& intf)
    {
        m_interfaces.erase(to_key(id, mac));
        if (id < m_port_interfaces.size()) {
            m_port_interfaces[id].erase(std::remove(std::begin(m_port_interfaces[id]),
                                                    std::end(m_port_interfaces[id]), intf),
                                        std::end(m_port_interfaces[id]));
        }
    }

    std::optional<Interface> unicast_find(uint16_t id, const net::mac_address& mac) const
    {
        auto item = m_interfaces.find(to_key(id, mac));
        return (item == m_interfaces.end()
                ? std::nullopt
                : std::make_optional(item->second));
    }

    std::optional<Interface> unicast_find(uint16_t id, const uint8_t octets[6]) const
    {
        auto item = m_interfaces.find(to_key(id, octets));
        return (item == m_interfaces.end()
                ? std::nullopt
                : std::make_optional(item->second));
    }

    bool have_port_interfaces(uint16_t id) const
    {
        return (id < m_port_interfaces.size());
    }

    const std::vector<Interface>& nunicast_find(uint16_t id) const
    {
        return (m_port_interfaces.at(id));
    }
};

}
}
}
#endif /* _ICP_PACKETIO_DPDK_SWITCH_H_ */
