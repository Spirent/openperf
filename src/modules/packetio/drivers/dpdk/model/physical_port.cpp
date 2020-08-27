#include "core/op_core.h"
#include "packetio/generic_port.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/drivers/dpdk/model/physical_port.hpp"

namespace openperf::packetio::dpdk::model {

physical_port::physical_port(int idx, std::string_view id, rte_mempool* pool)
    : m_idx(idx)
    , m_id(id)
    , m_pool(pool)
{
    if (!rte_eth_dev_is_valid_port(idx)) {
        throw std::runtime_error("Port id " + std::string(id) + " is invalid");
    }
}

std::string physical_port::id() const { return m_id; }

std::string physical_port::kind() { return "dpdk"; }

port::link_status physical_port::link() const
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(m_idx, &link);
    return (link.link_status == ETH_LINK_UP ? port::link_status::LINK_UP
                                            : port::link_status::LINK_DOWN);
}

port::link_speed physical_port::speed() const
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(m_idx, &link);
    return (link.link_status == ETH_LINK_UP
                ? static_cast<port::link_speed>(link.link_speed)
                : port::link_speed::SPEED_UNKNOWN);
}

port::link_duplex physical_port::duplex() const
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(m_idx, &link);
    return (link.link_duplex == ETH_LINK_FULL_DUPLEX
                ? port::link_duplex::DUPLEX_FULL
                : port::link_duplex::DUPLEX_HALF);
}

port::stats_data physical_port::stats() const
{
    struct rte_eth_stats stats;
    rte_eth_stats_get(m_idx, &stats);

    return {.rx_packets = static_cast<int64_t>(stats.ipackets),
            .tx_packets = static_cast<int64_t>(stats.opackets),
            .rx_bytes = static_cast<int64_t>(stats.ibytes),
            .tx_bytes = static_cast<int64_t>(stats.obytes),
            .rx_errors = static_cast<int64_t>(stats.imissed + stats.ierrors
                                              + stats.rx_nombuf),
            .tx_errors = static_cast<int64_t>(stats.oerrors)};
}

port::config_data physical_port::config() const
{
    auto info = model::port_info(m_idx);
    auto config = port::dpdk_config{.driver = info.driver_name(),
                                    .device = info.device_name(),
                                    .interface = info.interface_name()};

    rte_eth_link link;
    rte_eth_link_get_nowait(m_idx, &link);

    if (link.link_autoneg == ETH_LINK_AUTONEG) {
        config.auto_negotiation = true;
    } else {
        config.auto_negotiation = false;
        config.speed = static_cast<port::link_speed>(link.link_speed);
        config.duplex = (link.link_duplex == ETH_LINK_FULL_DUPLEX
                             ? port::link_duplex::DUPLEX_FULL
                             : port::link_duplex::DUPLEX_HALF);
    }

    return (config);
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

static __attribute__((const)) uint64_t filter_rx_offloads(uint64_t rx_capa)
{
    return (rx_capa
            & (DEV_RX_OFFLOAD_CHECKSUM | DEV_RX_OFFLOAD_JUMBO_FRAME
               | DEV_RX_OFFLOAD_SCATTER | DEV_RX_OFFLOAD_TCP_LRO));
}

static __attribute__((const)) uint64_t filter_tx_offloads(uint64_t tx_capa)
{
    return (tx_capa
            & (DEV_TX_OFFLOAD_IPV4_CKSUM | DEV_TX_OFFLOAD_UDP_CKSUM
               | DEV_TX_OFFLOAD_TCP_CKSUM | DEV_TX_OFFLOAD_TCP_TSO
               | DEV_TX_OFFLOAD_MULTI_SEGS));
}

static rte_eth_conf make_rte_eth_conf(const port_info& info)
{
    return {.link_speeds = ETH_LINK_SPEED_AUTONEG,
            .rxmode =
                {
                    .mq_mode = ETH_MQ_RX_RSS,
                    .max_rx_pkt_len = info.max_rx_pktlen(),
                    .offloads = filter_rx_offloads(info.rx_offloads()),
                },
            .txmode =
                {
                    .mq_mode = ETH_MQ_TX_NONE,
                    .offloads = filter_tx_offloads(info.tx_offloads()),
                },
            .rx_adv_conf = {.rss_conf =
                                {
                                    .rss_key = nullptr, /* use default */
                                    .rss_hf =
                                        info.rss_offloads() /* use whatever the
                                                               port supports */
                                }},
            .tx_adv_conf = {},
            .fdir_conf = {},
            .intr_conf = {.lsc = !!info.lsc_interrupt(),
                          .rxq = !!info.rxq_interrupt()}};
}

tl::expected<void, std::string>
physical_port::config(const port::config_data& c)
{
    if (!std::holds_alternative<port::dpdk_config>(c)) {
        return tl::make_unexpected("Config does not contain dpdk config data");
    }

    auto dpdk_config = std::get<port::dpdk_config>(c);

    /* Acquire some useful data about our port... */
    auto info = port_info(m_idx);

    rte_eth_conf port_conf = make_rte_eth_conf(info);

    /* Auto-negotiation is the default config. */
    if (dpdk_config.auto_negotiation) {
        return apply_port_config(info, port_conf);
    }

    /* Else, we need to check to see if the port supports the specified
     * speed/duplex */
    auto link_flag = eth_link_speed_flag(dpdk_config.speed, dpdk_config.duplex);
    if (!link_flag) {
        return (tl::make_unexpected(port::to_string(dpdk_config.speed) + " "
                                    + port::to_string(dpdk_config.duplex)
                                    + "-duplex is not a valid port speed"));
    }

    if ((link_flag & info.speeds()) == 0) {
        return (tl::make_unexpected(
            "Port does not support " + port::to_string(dpdk_config.speed) + " "
            + port::to_string(dpdk_config.duplex) + "-duplex"));
    }

    /* Update port config and then try to update port */
    port_conf.link_speeds = ETH_LINK_SPEED_FIXED | link_flag;
    return apply_port_config(info, port_conf);
}

tl::expected<void, std::string> physical_port::start()
{
    int error = rte_eth_dev_start(m_idx);
    if (error) {
        return tl::make_unexpected("Failed to start port " + m_id + "("
                                   + std::to_string(m_idx) + ")" + ": "
                                   + rte_strerror(error));
    }

    OP_LOG(OP_LOG_DEBUG,
           "Successfully started DPDK physical port %s\n",
           m_id.c_str());

    return {};
}

tl::expected<void, std::string> physical_port::stop()
{
    /*
     * The DPDK stop function doesn't free mbufs in use by hardware queues.
     * We need to call the tx,rx queue stop functions explicitly to do
     * that.
     */
    rte_eth_dev_info info;
    rte_eth_dev_info_get(m_idx, &info);

    std::vector<std::string> errors;
    for (int q = 0; q < info.nb_tx_queues; q++) {
        int error = rte_eth_dev_tx_queue_stop(m_idx, q);
        if (error && error != -ENOTSUP) {
            errors.emplace_back("Failed to cleanly stop TX queue "
                                + std::to_string(q) + " on port " + m_id + "("
                                + std::to_string(m_idx) + ")" + ": "
                                + rte_strerror(error) + ".");
        }
    }

    for (int q = 0; q < info.nb_rx_queues; q++) {
        int error = rte_eth_dev_rx_queue_stop(m_idx, q);
        if (error && error != -ENOTSUP) {
            errors.emplace_back("Failed to cleanly stop RX queue "
                                + std::to_string(q) + " on port " + m_id + "("
                                + std::to_string(m_idx) + ")" + ": "
                                + rte_strerror(error) + ".");
        }
    }

    if (errors.empty()) {
        rte_eth_dev_stop(m_idx);
        return {};
    }

    OP_LOG(OP_LOG_DEBUG,
           "Successfully stopped DPDK physical port %s\n",
           m_id.c_str());

    return (tl::make_unexpected(std::accumulate(
        begin(errors),
        end(errors),
        std::string(),
        [](const std::string& a, const std::string& b) -> std::string {
            return a + (a.length() > 0 ? " " : "") + b;
        })));
}

tl::expected<void, std::string>
physical_port::apply_port_config(port_info& info,
                                 rte_eth_conf& port_conf,
                                 uint16_t nb_rxqs,
                                 uint16_t nb_txqs)
{
    nb_rxqs = (nb_rxqs == 0 ? info.rx_queue_default() : nb_rxqs);
    nb_txqs = (nb_txqs == 0 ? info.tx_queue_default() : nb_txqs);
    bool do_start = false;

    int error = rte_eth_dev_configure(m_idx, nb_rxqs, nb_txqs, &port_conf);
    if (error == -EBUSY) {
        /* The port is still running; stop it and try again. */
        auto stop_result = stop();
        if (!stop_result) { return stop_result; }
        do_start =
            true; /* Assuming we make it to the end without additional errors */
        error = rte_eth_dev_configure(m_idx, nb_rxqs, nb_txqs, &port_conf);
    }

    if (error) {
        return (tl::make_unexpected("Failed to configure port " + m_id + "("
                                    + std::to_string(m_idx) + ")" + ": "
                                    + rte_strerror(error)));
    }

    /* Setup Tx queues */
    rte_eth_txconf tx_conf = info.default_txconf();
    tx_conf.offloads = port_conf.txmode.offloads;

    for (int q = 0; q < info.tx_queue_count(); q++) {
        if ((error = rte_eth_tx_queue_setup(
                 m_idx, q, info.tx_desc_count(), info.socket_id(), &tx_conf))
            != 0) {
            return (tl::make_unexpected("Failed to setup TX queue "
                                        + std::to_string(q) + " on port " + m_id
                                        + "( " + std::to_string(m_idx)
                                        + "): " + rte_strerror(error)));
        }
    }

    /* Setup Rx queues */
    rte_eth_rxconf rx_conf = info.default_rxconf();
    rx_conf.offloads = port_conf.rxmode.offloads;

    for (int q = 0; q < info.rx_queue_count(); q++) {
        if ((error = rte_eth_rx_queue_setup(m_idx,
                                            q,
                                            info.rx_desc_count(),
                                            info.socket_id(),
                                            &rx_conf,
                                            const_cast<rte_mempool*>(m_pool)))
            != 0) {
            return (tl::make_unexpected("Failed to setup RX queue "
                                        + std::to_string(q) + " on port " + m_id
                                        + "(" + std::to_string(m_idx)
                                        + "): " + rte_strerror(error)));
        }
    }

    OP_LOG(OP_LOG_DEBUG,
           "Successfully configured DPDK physical port %s "
           "(idx=%u, rxq=%d, txq=%d, pool=%s, speed=0x%x)\n",
           m_id.c_str(),
           m_idx,
           nb_rxqs,
           nb_txqs,
           m_pool->name,
           port_conf.link_speeds);

    if (do_start) return start();
    return {};
}

tl::expected<void, std::string>
physical_port::low_level_config(uint16_t nb_rxqs, uint16_t nb_txqs)
{
    auto info = port_info(m_idx);

    rte_eth_conf port_conf = make_rte_eth_conf(info);

    auto result = apply_port_config(info, port_conf, nb_rxqs, nb_txqs);
    if (!result) { return (result); }

    return {};
}

} // namespace openperf::packetio::dpdk::model
