#include "config/op_config_file.hpp"
#include "packetio/drivers/dpdk/port_info.hpp"
#include "packetio/drivers/dpdk/primary/arg_parser.hpp"

namespace openperf::packetio::dpdk::port_info {

uint16_t rx_queue_max(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::max_rx_queues));
}

uint16_t rx_queue_default(uint16_t port_id)
{
    return (
        std::max(static_cast<uint16_t>(1),
                 get_info_field(port_id, &rte_eth_dev_info::default_rxportconf)
                     .nb_queues));
}

uint16_t tx_queue_count(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::nb_tx_queues));
}

uint16_t tx_queue_max(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::max_rx_queues));
}

uint16_t tx_queue_default(uint16_t port_id)
{
    return (
        std::max(static_cast<uint16_t>(1),
                 get_info_field(port_id, &rte_eth_dev_info::default_txportconf)
                     .nb_queues));
}

bool lsc_interrupt(uint16_t port_id)
{
    auto no_lsc_irq =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_NONE>(
            op_packetio_dpdk_no_link_irqs)
            .value_or(false);
    if (no_lsc_irq) { return (false); }

    return (*(get_info_field(port_id, &rte_eth_dev_info::dev_flags))
            & RTE_ETH_DEV_INTR_LSC);
}

} // namespace openperf::packetio::dpdk::port_info
