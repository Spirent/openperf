#include "config/op_config_file.hpp"
#include "packetio/drivers/dpdk/port_info.hpp"
#include "packetio/drivers/dpdk/secondary/arg_parser.hpp"

namespace openperf::packetio::dpdk::port_info {

uint16_t rx_queue_max(uint16_t port_id) { return (rx_queue_count(port_id)); }

uint16_t rx_queue_default(uint16_t port_id)
{
    return (rx_queue_count(port_id));
}

uint16_t tx_queue_count(uint16_t port_id)
{
    auto port_queues = config::tx_port_queues();
    return (port_queues.count(port_id) ? port_queues[port_id].size() : 0);
}

uint16_t tx_queue_max(uint16_t port_id) { return (tx_queue_count(port_id)); }

uint16_t tx_queue_default(uint16_t port_id)
{
    return (tx_queue_count(port_id));
}

bool lsc_interrupt(uint16_t) { return (false); }

} // namespace openperf::packetio::dpdk::port_info
