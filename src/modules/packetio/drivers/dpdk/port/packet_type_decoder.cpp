#include <algorithm>
#include <string>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/drivers/dpdk/port/packet_type_decoder.hpp"

namespace openperf::packetio::dpdk::port {

inline constexpr uint32_t decode_mask =
    RTE_PTYPE_L2_MASK | RTE_PTYPE_L3_MASK | RTE_PTYPE_L4_MASK;

static uint16_t decode_packet_types([[maybe_unused]] uint16_t port_id,
                                    [[maybe_unused]] uint16_t queue_id,
                                    rte_mbuf* packets[],
                                    uint16_t nb_packets,
                                    [[maybe_unused]] uint16_t max_packets,
                                    [[maybe_unused]] void* user_param)
{
    std::for_each(packets, packets + nb_packets, [&](auto&& mbuf) {
        mbuf->packet_type = rte_net_get_ptype(mbuf, nullptr, decode_mask);
    });

    return (nb_packets);
}

callback_packet_type_decoder::callback_packet_type_decoder(uint16_t port_id)
    : m_port(port_id)
{}

callback_packet_type_decoder::~callback_packet_type_decoder()
{
    std::for_each(
        std::begin(m_callbacks), std::end(m_callbacks), [&](auto& item) {
            rte_eth_remove_rx_callback(port_id(), item.first, item.second);
        });
}

callback_packet_type_decoder::callback_packet_type_decoder(
    callback_packet_type_decoder&& other) noexcept
    : m_callbacks(std::move(other.m_callbacks))
    , m_port(other.m_port)
{}

callback_packet_type_decoder& callback_packet_type_decoder::operator=(
    callback_packet_type_decoder&& other) noexcept
{
    if (this != &other) {
        m_callbacks = std::move(other.m_callbacks);
        m_port = other.m_port;
    }

    return (*this);
}

uint16_t callback_packet_type_decoder::port_id() const { return (m_port); }

void callback_packet_type_decoder::enable()
{
    OP_LOG(OP_LOG_INFO,
           "Enabling software packet type decoding on port %u\n",
           m_port);

    auto q = 0U;
    std::generate_n(std::inserter(m_callbacks, std::begin(m_callbacks)),
                    model::port_info(port_id()).rx_queue_count(),
                    [&]() {
                        auto cb = rte_eth_add_rx_callback(
                            port_id(), q, decode_packet_types, nullptr);
                        if (!cb) {
                            throw std::runtime_error(
                                "Could not add ptype decode callback to port "
                                + std::to_string(port_id()) + ", queue "
                                + std::to_string(q));
                        }
                        return (std::make_pair(q++, cb));
                    });
}

void callback_packet_type_decoder::disable()
{
    OP_LOG(OP_LOG_INFO,
           "Disabling software packet type decoding on port %u\n",
           m_port);

    std::for_each(
        std::begin(m_callbacks), std::end(m_callbacks), [&](auto&& item) {
            rte_eth_remove_rx_callback(port_id(), item.first, item.second);
        });

    m_callbacks.clear();
}

static packet_type_decoder::variant_type
make_packet_type_decoder(uint16_t port_id)
{
    if (rte_eth_dev_get_supported_ptypes(port_id, decode_mask, nullptr, 0)
        <= 0) {
        return (callback_packet_type_decoder(port_id));
    }

    return (null_feature(port_id));
}

packet_type_decoder::packet_type_decoder(uint16_t port_id)
    : feature(make_packet_type_decoder(port_id))
{}

} // namespace openperf::packetio::dpdk::port
