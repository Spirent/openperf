#ifndef _OP_PACKETIO_FORWARDING_TABLE_HPP_
#define _OP_PACKETIO_FORWARDING_TABLE_HPP_

/**
 * @file
 *
 * The forwarding_table contains all of the details needed to dispatch
 * packets on the receive path.
 */

#include <array>
#include <atomic>

#include "immer/box.hpp"
#include "immer/flex_vector.hpp"
#include "immer/map.hpp"

#include "packet/type/mac_address.hpp"

namespace openperf::packetio {

template <typename Interface, typename Sink, int MaxPorts>
class forwarding_table
{
public:
    enum class direction { RX, TX };

    struct interface_sinks
    {
        Interface ifp;
        std::vector<Sink> rx_sinks;
        std::vector<Sink> tx_sinks;
    };

    using interface_sink_entry = immer::box<interface_sinks>;
    using interface_map =
        immer::map<libpacket::type::mac_address, interface_sink_entry>;
    using sink_vector = immer::flex_vector<Sink>;
    static constexpr unsigned mac_address_length =
        libpacket::type::mac_address{}.size();

    forwarding_table();
    ~forwarding_table();

    interface_map* insert_interface(uint16_t port_idx,
                                    const libpacket::type::mac_address& mac,
                                    Interface interface);
    interface_map* remove_interface(uint16_t port_idx,
                                    const libpacket::type::mac_address& mac);

    sink_vector* insert_sink(uint16_t port_idx, direction dir, Sink sink);
    sink_vector* remove_sink(uint16_t port_idx, direction dir, Sink sink);

    interface_map*
    insert_interface_sink(uint16_t port_idx,
                          const libpacket::type::mac_address& mac,
                          direction dir,
                          Sink sink);
    interface_map*
    remove_interface_sink(uint16_t port_idx,
                          const libpacket::type::mac_address& mac,
                          direction dir,
                          Sink sink);

    const Interface* find_interface(uint16_t port_idx,
                                    std::string_view id) const;
    const Interface* find_interface(std::string_view id) const;

    const Interface*
    find_interface(uint16_t port_idx,
                   const libpacket::type::mac_address& mac) const;
    const Interface*
    find_interface(uint16_t port_idx,
                   const uint8_t octets[mac_address_length]) const;

    interface_map& get_interfaces(uint16_t port_idx) const;

    sink_vector& get_rx_sinks(uint16_t port_idx) const;

    sink_vector& get_tx_sinks(uint16_t port_idx) const;

    bool has_interface_rx_sinks(uint16_t port_idx) const;

    bool has_interface_tx_sinks(uint16_t port_idx) const;

    const std::vector<Sink>*
    find_interface_tx_sinks(uint16_t port_idx,
                            const libpacket::type::mac_address& mac) const;
    const std::vector<Sink>*
    find_interface_rx_sinks(uint16_t port_idx,
                            const libpacket::type::mac_address& mac) const;
    const interface_sinks*
    find_interface_and_sinks(uint16_t port_idx,
                             const libpacket::type::mac_address& mac) const;

    /**
     * Visit all the interfaces sinks.
     *
     * This function is not intended for use in the fast path packet dispatch.
     * Due to std::function performance overhead it is better to use
     * get_interfaces().
     */
    void visit_interface_sinks(
        uint16_t port_idx,
        direction direction,
        std::function<bool(const Interface& ifp, const Sink& sink)>&& visitor)
        const;

private:
    std::array<std::atomic<interface_map*>, MaxPorts> m_interfaces;
    std::array<std::atomic<sink_vector*>, MaxPorts> m_port_rx_sinks;
    std::array<std::atomic<sink_vector*>, MaxPorts> m_port_tx_sinks;
    std::array<std::atomic<int>, MaxPorts> m_port_interface_rx_sink_count;
    std::array<std::atomic<int>, MaxPorts> m_port_interface_tx_sink_count;
};

} // namespace openperf::packetio

#endif /* _OP_PACKETIO_FORWARDING_TABLE_HPP_ */
