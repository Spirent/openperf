#include "config/op_config_file.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/port_info.hpp"
#include "packetio/drivers/dpdk/quirks.hpp"
#include "packetio/drivers/dpdk/primary/arg_parser.hpp"
#include "packetio/drivers/dpdk/primary/utils.hpp"

namespace openperf::packetio::dpdk::primary::utils {

tl::expected<void, std::string> start_port(uint16_t port_id)
{
    if (auto error = rte_eth_dev_start(port_id)) {
        return (tl::make_unexpected("Failed to start port "
                                    + std::to_string(port_id) + ": "
                                    + rte_strerror(std::abs(error))));
    }

    return {};
}

tl::expected<void, std::string> stop_port(uint16_t port_id)
{
    /*
     * The DPDK stop function doesn't free mbufs in use by hardware queues.
     * We need to call the tx,rx queue stop functions explicitly to do
     * that.
     */
    auto info = rte_eth_dev_info{};
    rte_eth_dev_info_get(port_id, &info);

    auto errors = std::vector<std::string>{};
    for (int q = 0; q < info.nb_tx_queues; q++) {
        int error = rte_eth_dev_tx_queue_stop(port_id, q);
        if (error && error != -ENOTSUP) {
            errors.emplace_back("Failed to cleanly stop TX queue "
                                + std::to_string(q) + " on port "
                                + std::to_string(port_id) + ": "
                                + rte_strerror(std::abs(error)) + ".");
        }
    }

    for (int q = 0; q < info.nb_rx_queues; q++) {
        int error = rte_eth_dev_rx_queue_stop(port_id, q);
        if (error && error != -ENOTSUP) {
            errors.emplace_back("Failed to cleanly stop RX queue "
                                + std::to_string(q) + " on port "
                                + std::to_string(port_id) + ": "
                                + rte_strerror(std::abs(error)) + ".");
        }
    }

    if (!errors.empty()) {
        return (tl::make_unexpected(
            std::accumulate(begin(errors),
                            end(errors),
                            std::string{},
                            [](std::string& a, const std::string& b) {
                                a += (a.length() > 0 ? " " : "") + b;
                                return (a);
                            })));
    }

    if (auto error = rte_eth_dev_stop(port_id)) {
        return (tl::make_unexpected("Failed to stop port "
                                    + std::to_string(port_id) + ":"
                                    + rte_strerror(std::abs(error))));
    }

    return {};
}

static uint32_t eth_link_speed_flag(port::link_speed speed,
                                    port::link_duplex duplex)
{
    static std::unordered_map<port::link_speed, uint32_t> hd_flags = {
        {port::link_speed::SPEED_10M, ETH_LINK_SPEED_10M_HD},
        {port::link_speed::SPEED_100M, ETH_LINK_SPEED_100M_HD},
    };

    static std::unordered_map<port::link_speed, uint32_t> fd_flags = {
        {port::link_speed::SPEED_10M, ETH_LINK_SPEED_10M},
        {port::link_speed::SPEED_100M, ETH_LINK_SPEED_100M},
        {port::link_speed::SPEED_1G, ETH_LINK_SPEED_1G},
        {port::link_speed::SPEED_2_5G, ETH_LINK_SPEED_2_5G},
        {port::link_speed::SPEED_5G, ETH_LINK_SPEED_5G},
        {port::link_speed::SPEED_10G, ETH_LINK_SPEED_10G},
        {port::link_speed::SPEED_20G, ETH_LINK_SPEED_20G},
        {port::link_speed::SPEED_25G, ETH_LINK_SPEED_25G},
        {port::link_speed::SPEED_40G, ETH_LINK_SPEED_40G},
        {port::link_speed::SPEED_50G, ETH_LINK_SPEED_50G},
        {port::link_speed::SPEED_56G, ETH_LINK_SPEED_56G},
        {port::link_speed::SPEED_100G, ETH_LINK_SPEED_100G}};

    if (duplex == port::link_duplex::DUPLEX_HALF) {
        return (hd_flags.find(speed) != hd_flags.end() ? hd_flags.at(speed)
                                                       : 0);
    }

    return (fd_flags.find(speed) != fd_flags.end() ? fd_flags.at(speed) : 0);
}

static uint64_t lro_flag()
{
    return (config::dpdk_disable_lro() ? 0 : RTE_ETH_RX_OFFLOAD_TCP_LRO);
}

static uint64_t filter_rx_offloads(uint64_t rx_capa)
{
    return (rx_capa
            & (RTE_ETH_RX_OFFLOAD_CHECKSUM | RTE_ETH_RX_OFFLOAD_SCATTER
               | lro_flag()));
}

static constexpr uint64_t filter_tx_offloads(uint64_t tx_capa)
{
    return (tx_capa
            & (RTE_ETH_TX_OFFLOAD_IPV4_CKSUM | RTE_ETH_TX_OFFLOAD_UDP_CKSUM
               | RTE_ETH_TX_OFFLOAD_TCP_CKSUM | RTE_ETH_TX_OFFLOAD_TCP_TSO
               | RTE_ETH_TX_OFFLOAD_MULTI_SEGS));
}

static rte_eth_conf make_rte_eth_conf(uint16_t port_id)
{
    return {
        .link_speeds = ETH_LINK_SPEED_AUTONEG,
        .rxmode =
            {
                .mtu = port_info::default_rx_mtu(port_id),
                .mq_mode = port_info::rx_mq_mode(port_id),
                .max_lro_pkt_size = port_info::max_lro_pkt_size(port_id)
                                    - quirks::adjust_max_rx_pktlen(port_id),
                .offloads = filter_rx_offloads(port_info::rx_offloads(port_id)),
            },
        .txmode =
            {
                .mq_mode = ETH_MQ_TX_NONE,
                .offloads = filter_tx_offloads(port_info::tx_offloads(port_id)),
            },
        .rx_adv_conf = {.rss_conf =
                            {
                                .rss_key = nullptr, /* use default */
                                .rss_hf = port_info::rss_offloads(
                                    port_id) /* use whatever the
                                   port supports */
                            }},
        .tx_adv_conf = {},
        .fdir_conf = {},
        .intr_conf = {.lsc = !!port_info::lsc_interrupt(port_id),
                      .rxq = !config::dpdk_disable_rx_irq()}};
}

static tl::expected<void, std::string>
do_port_config(uint16_t port_id,
               const rte_eth_conf& config,
               rte_mempool* const mempool,
               uint16_t nb_rxqs,
               uint16_t nb_txqs)
{
    bool do_start = false;

    auto error = rte_eth_dev_configure(port_id, nb_rxqs, nb_txqs, &config);
    if (error == -EBUSY) {
        /* The port is still running; stop it and try again */
        if (auto result = stop_port(port_id); !result) { return (result); }
        do_start = true; /* restart is necessary */
        error = rte_eth_dev_configure(port_id, nb_rxqs, nb_txqs, &config);
    }

    if (error) {
        return (tl::make_unexpected("Failed to configure port "
                                    + std::to_string(port_id) + ": "
                                    + rte_strerror(std::abs(error))));
    }

    /* Setup Tx queues */
    auto tx_conf = port_info::default_txconf(port_id);
    tx_conf.offloads = config.txmode.offloads;

    for (auto q = 0U; q < nb_txqs; q++) {
        if (auto tx_error =
                rte_eth_tx_queue_setup(port_id,
                                       q,
                                       port_info::tx_desc_count(port_id),
                                       port_info::socket_id(port_id),
                                       &tx_conf)) {
            return (tl::make_unexpected("Failed to setup TX queue on port "
                                        + std::to_string(port_id) + ": "
                                        + rte_strerror(std::abs(error))));
        }
    }

    /* Now, setup RX queues */
    auto rx_conf = port_info::default_rxconf(port_id);
    rx_conf.offloads = config.rxmode.offloads;

    for (auto q = 0U; q < nb_rxqs; q++) {
        if (auto rx_error =
                rte_eth_rx_queue_setup(port_id,
                                       q,
                                       port_info::rx_desc_count(port_id),
                                       port_info::socket_id(port_id),
                                       &rx_conf,
                                       mempool)) {
            return (tl::make_unexpected("Failed to setup RX queue on port "
                                        + std::to_string(port_id) + ": "
                                        + rte_strerror(std::abs(error))));
        }
    }

    if (do_start) { return start_port(port_id); }

    return {};
}

tl::expected<void, std::string> configure_port(uint16_t port_id,
                                               rte_mempool* const mempool,
                                               uint16_t nb_rxqs,
                                               uint16_t nb_txqs)
{
    return (do_port_config(
        port_id, make_rte_eth_conf(port_id), mempool, nb_rxqs, nb_txqs));
}

/* XXX: This function assumes the port has already been configured once */
tl::expected<void, std::string>
reconfigure_port(uint16_t port_id,
                 rte_mempool* const mempool,
                 const port::dpdk_config& config)
{
    auto port_conf = make_rte_eth_conf(port_id);

    /* Auto-negotiation is the default config. */
    if (config.auto_negotiation) {
        return (do_port_config(port_id,
                               port_conf,
                               mempool,
                               port_info::rx_queue_count(port_id),
                               port_info::tx_queue_count(port_id)));
    }

    /* Else, we need to check to see if the port supports the specified
     * speed/duplex */
    auto link_flag = eth_link_speed_flag(config.speed, config.duplex);
    if (!link_flag) {
        return (tl::make_unexpected(port::to_string(config.speed) + " "
                                    + port::to_string(config.duplex)
                                    + "-duplex is not a valid port speed"));
    }

    if ((link_flag & port_info::speeds(port_id)) == 0) {
        return (tl::make_unexpected(
            "Port does not support " + port::to_string(config.speed) + " "
            + port::to_string(config.duplex) + "-duplex"));
    }

    /* Update port config and then try to update port */
    port_conf.link_speeds = ETH_LINK_SPEED_FIXED | link_flag;
    return (do_port_config(port_id,
                           port_conf,
                           mempool,
                           port_info::rx_queue_count(port_id),
                           port_info::tx_queue_count(port_id)));
}

} // namespace openperf::packetio::dpdk::primary::utils
