#include <assert.h>

#include "core/icp_core.h"
#include "packetio_dpdk.h"
#include "packetio_port.h"


const static struct rte_fdir_conf fdir_conf = {
    .mode = RTE_FDIR_MODE_NONE,
    .pballoc = RTE_FDIR_PBALLOC_64K,
    .status = RTE_FDIR_REPORT_STATUS,
    .mask = {
        .vlan_tci_mask = 0x0,
        .ipv4_mask     = {
            .src_ip = 0xFFFFFFFF,
            .dst_ip = 0xFFFFFFFF,
        },
        .ipv6_mask     = {
            .src_ip = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
            .dst_ip = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
        },
        .src_port_mask = 0xFFFF,
        .dst_port_mask = 0xFFFF,
        .mac_addr_byte_mask = 0xFF,
        .tunnel_type_mask = 1,
        .tunnel_id_mask = 0xFFFFFFFF,
    },
    .drop_queue = 127,
};

static
void populate_dpdk_txconf(uint16_t port_idx,
                          struct rte_eth_txconf *txconf)
{
    assert(txconf);

    /* Get the port's capabilities so we can intelligently decide what to do */
    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(port_idx, &info);
    *txconf = info.default_txconf;

    /* We want checksumming, so turn it on if the port supports it. */
    if (info.tx_offload_capa & DEV_TX_OFFLOAD_IPV4_CKSUM) {
        txconf->offloads |= DEV_TX_OFFLOAD_IPV4_CKSUM;
    }

    if (info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM) {
        txconf->offloads |= DEV_TX_OFFLOAD_TCP_CKSUM;
    }

    if (info.tx_offload_capa & DEV_TX_OFFLOAD_UDP_CKSUM) {
        txconf->offloads |= DEV_TX_OFFLOAD_UDP_CKSUM;
    }

    /* This seems useful, too */
    if (info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE) {
        txconf->offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
    }

    icp_log(ICP_LOG_INFO, "Port %d supported offloads: tx = 0x%"PRIx64", rx = 0x%"PRIx64"\n",
            port_idx, info.tx_offload_capa, info.rx_offload_capa);
    icp_log(ICP_LOG_INFO, "Using Tx offloads = 0x%"PRIx64" on port %d\n", txconf->offloads, port_idx);
}

static
void populate_dpdk_rxmode(uint16_t port_idx,
                          struct rte_eth_rxmode *rxmode)
{
    /* Get the port's capapbilities so we can intelligently decide what to do */
    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(port_idx, &info);

    rxmode->max_rx_pkt_len = ETHER_MAX_LEN;
    rxmode->split_hdr_size = 0;
    rxmode->offloads = 0;

    /* Enable checksumming if the port supports it */
    if (info.tx_offload_capa & DEV_RX_OFFLOAD_CHECKSUM) {
        rxmode->offloads |= DEV_RX_OFFLOAD_CHECKSUM;
    }

    /* Keep the CRC, if possible */
    if (info.rx_offload_capa & DEV_RX_OFFLOAD_KEEP_CRC) {
        rxmode->offloads |= DEV_RX_OFFLOAD_KEEP_CRC;
    }

    if (info.rx_offload_capa & DEV_RX_OFFLOAD_SCATTER) {
        rxmode->offloads |= DEV_RX_OFFLOAD_SCATTER;
    }

    icp_log(ICP_LOG_INFO, "Using Rx offloads = 0x%"PRIx64" on port %d\n", rxmode->offloads, port_idx);
}

static void
_populate_rte_eth_conf(uint16_t port_idx, uint16_t nb_rx_queues, struct rte_eth_conf *conf)
{
    memset(conf, 0, sizeof(*conf));

    populate_dpdk_rxmode(port_idx, &conf->rxmode);
    conf->fdir_conf = fdir_conf;

    if (nb_rx_queues > 1) {
        conf->rx_adv_conf.rss_conf.rss_key = NULL;
        conf->rx_adv_conf.rss_conf.rss_key_len = 0;
        conf->rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IP | ETH_RSS_TCP | ETH_RSS_UDP;
    } else {
        conf->rx_adv_conf.rss_conf.rss_key = NULL;
        conf->rx_adv_conf.rss_conf.rss_key_len = 0;
        conf->rx_adv_conf.rss_conf.rss_hf = 0;
    }

    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(port_idx, &info);

    if (info.max_vfs == 0) {
        conf->rxmode.mq_mode = (
            conf->rx_adv_conf.rss_conf.rss_hf ? ETH_MQ_RX_RSS : ETH_MQ_RX_NONE);
        conf->txmode.mq_mode = ETH_MQ_TX_NONE;
    } else {
        conf->rxmode.mq_mode = (
            conf->rx_adv_conf.rss_conf.rss_hf ? ETH_MQ_RX_VMDQ_RSS : ETH_MQ_RX_NONE);
        conf->txmode.mq_mode = ETH_MQ_TX_VMDQ_ONLY;
    }

    /* Enable interrupts maybe? */
    conf->intr_conf.lsc = !!packetio_port_config_lsc_interrupt(port_idx);
    //conf->intr_conf.rxq = !!packetio_port_config_rxq_interrupt(port_idx);
}

__attribute__((weak))
uint8_t packetio_port_count()
{
    return (rte_eth_dev_count_avail());
}

#define DPDK_PORT_INFO_GETTER(type, field)        \
    __attribute__((weak))                         \
    type packetio_port_##field(uint16_t port_idx)	\
    {                                             \
        struct rte_eth_dev_info info;             \
        rte_eth_dev_info_get(port_idx, &info);		\
        return (info.field);                      \
    }

DPDK_PORT_INFO_GETTER(const char *, driver_name)
DPDK_PORT_INFO_GETTER(uint16_t, max_rx_queues)
DPDK_PORT_INFO_GETTER(uint16_t, max_tx_queues)
DPDK_PORT_INFO_GETTER(uint32_t, min_rx_bufsize)
DPDK_PORT_INFO_GETTER(uint16_t, nb_rx_queues)
DPDK_PORT_INFO_GETTER(uint16_t, nb_tx_queues)
DPDK_PORT_INFO_GETTER(uint32_t, tx_offload_capa)

#undef DPDK_PORT_INFO_GETTER

__attribute__((weak))
uint32_t packetio_port_speed(uint16_t port_idx)
{
    /* If the port has link, return the current speed */
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_idx, &link);
    if (link.link_status == ETH_LINK_UP) {
        return (link.link_speed);
    }

    /* If there is no link, see if the port knows about it's speeds */
    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(port_idx, &info);

    if (info.speed_capa & ETH_LINK_SPEED_100G) {
        return (ETH_SPEED_NUM_100G);
    } else if (info.speed_capa & ETH_LINK_SPEED_56G) {
        return (ETH_SPEED_NUM_56G);
    } else if (info.speed_capa & ETH_LINK_SPEED_50G) {
        return (ETH_SPEED_NUM_50G);
    } else if (info.speed_capa & ETH_LINK_SPEED_40G) {
        return (ETH_SPEED_NUM_40G);
    } else if (info.speed_capa & ETH_LINK_SPEED_25G) {
        return (ETH_SPEED_NUM_25G);
    } else if (info.speed_capa & ETH_LINK_SPEED_20G) {
        return (ETH_SPEED_NUM_20G);
    } else if (info.speed_capa & ETH_LINK_SPEED_10G) {
        return (ETH_SPEED_NUM_10G);
    } else if (info.speed_capa & ETH_LINK_SPEED_5G) {
        return (ETH_SPEED_NUM_5G);
    } else if (info.speed_capa & ETH_LINK_SPEED_2_5G) {
        return (ETH_SPEED_NUM_2_5G);
    } else if (info.speed_capa & ETH_LINK_SPEED_1G) {
        return (ETH_SPEED_NUM_1G);
    } else if (info.speed_capa & ETH_LINK_SPEED_100M
               || info.speed_capa & ETH_LINK_SPEED_100M_HD) {
        return (ETH_SPEED_NUM_100M);
    } else if (info.speed_capa & ETH_LINK_SPEED_10M
               || info.speed_capa & ETH_LINK_SPEED_10M_HD) {
        return (ETH_SPEED_NUM_10M);
    } else {
        /*
         * If we don't have any speed capabilities, return the link speed.
         * It might be hard coded.
         */
        return (link.link_speed ? link.link_speed : ETH_SPEED_NUM_NONE);
    }
}

int packetio_port_link_up(uint16_t port_idx)
{
    return (rte_eth_dev_set_link_up(port_idx));
}

int packetio_port_link_down(uint16_t port_idx)
{
    return (rte_eth_dev_set_link_down(port_idx));
}

bool packetio_port_link_check(uint16_t port_idx)
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_idx, &link);
    return (link.link_status == ETH_LINK_UP);
}

void packetio_port_link_log(uint16_t port_idx)
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_idx, &link);
    if (link.link_status == ETH_LINK_UP) {
        icp_log(ICP_LOG_INFO, "Port %u Link Up - speed %u Mbps - %s-duplex\n",
                port_idx, link.link_speed,
                link.link_duplex == ETH_LINK_FULL_DUPLEX ? "full" : "half");
    } else {
        icp_log(ICP_LOG_INFO, "Port %u Link Down\n", port_idx);
    }
}

__attribute__((weak))
int packetio_port_mac(uint16_t port_idx, union mac_address *mac)
{
    if (!rte_eth_dev_is_valid_port(port_idx)) {
        return (-ENODEV);
    }

    /* XXX: Our mac address union is congruent with DPDK's ether_addr struct */
    rte_eth_macaddr_get(port_idx, (struct ether_addr *)mac);

    return (0);
}

int packetio_port_start(uint16_t port_idx)
{
    int error = rte_eth_dev_start(port_idx);
    if (error) {
        icp_log(ICP_LOG_ERROR, "Failed to start port %d: %s\n",
                port_idx, rte_strerror(error));
    } else {
        icp_log(ICP_LOG_INFO, "Started port %d\n", port_idx);
    }
    return (error);
}

/*
 * The DPDK stop function doesn't free mbufs in use by hardware queues.
 * We need to call the tx,rx queue stop functions explicitly to do that.
 */
int packetio_port_stop(uint16_t port_idx)
{
    if (port_idx >= packetio_port_count()) {
        return (-ENODEV);
    }

    uint16_t nb_txqs = packetio_port_nb_tx_queues(port_idx);
    uint16_t nb_rxqs = packetio_port_nb_rx_queues(port_idx);
    int error = 0;
    bool errors = false;

    /* Stop TX queue(s) */
    for (uint16_t q = 0; q < nb_txqs; q++) {
        error = rte_eth_dev_tx_queue_stop(port_idx, q);
        if (error && error != -ENOTSUP) {
            icp_log(ICP_LOG_ERROR, "Failed to cleanly stop TX queue %d on port %d: %s\n",
                    q, port_idx, strerror(error));
            errors = true;
        }
    }

    /* Stop RX queue(s) */
    for (uint16_t q = 0; q < nb_rxqs; q++) {
        error = rte_eth_dev_rx_queue_stop(port_idx, q);
        if (error && error != -ENOTSUP) {
            icp_log(ICP_LOG_ERROR, "Failed to cleanly stop RX queue %d on port %d: %s\n",
                    q, port_idx, strerror(error));
            errors = true;
        }
    }

    /* Finally, stop the port */
    rte_eth_dev_stop(port_idx);

    return (errors ? -1 : 0);
}

int packetio_port_configure(uint16_t port_idx,
                            uint16_t nb_rxqs, uint16_t nb_txqs,
                            struct rte_mempool *rx_pool)
{
    assert(rx_pool);
    icp_log(ICP_LOG_INFO, "port %d, rxqs = %d, txqs = %d, pool = %p\n",
            port_idx, nb_rxqs, nb_txqs, (void *)rx_pool);
    /* Ports require at least 1 queue */
    if (nb_rxqs == 0 && nb_txqs == 0) {
        return (-EINVAL);
    }

    /* Ensure port is in stopped state */
    packetio_port_stop(port_idx);

    /* Configure the port */
    struct rte_eth_conf conf = {};
    _populate_rte_eth_conf(port_idx, nb_rxqs, &conf);

    int error = 0;
    if ((error = rte_eth_dev_configure(port_idx,
                                       nb_rxqs, nb_txqs,
                                       &conf)) != 0) {
        icp_log(ICP_LOG_ERROR, "Failed to configure port %d: %s\n",
                port_idx, rte_strerror(error));
        return (error);
    }

    /* Setup TX queue(s) */
    struct rte_eth_txconf txconf = {};
    populate_dpdk_txconf(port_idx, &txconf);
    uint16_t nb_tx_desc = packetio_port_config_tx_desc_count(port_idx);
    for (uint16_t q = 0; q < nb_txqs; q++) {
        if ((error = rte_eth_tx_queue_setup(port_idx, q,
                                            nb_tx_desc,
                                            SOCKET_ID_ANY,
                                            &txconf)) != 0) {
            icp_log(ICP_LOG_ERROR, "Failed to setup tx queue %d on port %d: %s\n",
                    q, port_idx, rte_strerror(error));
            return (error);
        }
    }

    /* Setup RX queue(s) */
    uint16_t nb_rx_desc = packetio_port_config_rx_desc_count(port_idx);
    for (uint16_t q = 0; q < nb_rxqs; q++) {
        if ((error = rte_eth_rx_queue_setup(port_idx, q,
                                            nb_rx_desc,
                                            SOCKET_ID_ANY,
                                            NULL,
                                            rx_pool)) != 0) {
            icp_log(ICP_LOG_ERROR, "Failed to setup rx queue %d on port %d: %s\n",
                    q, port_idx, rte_strerror(error));
            return (error);
        }
    }

    /* (re)start the port */
    packetio_port_start(port_idx);
    rte_eth_promiscuous_enable(port_idx);
    return (0);
}

static
int lsc_interrupt_callback(uint16_t port_idx,
                            enum rte_eth_event_type type __attribute__((unused)),
                            void *param __attribute__((unused)),
                            void *ret_param __attribute__((unused)))
{
    assert(type == RTE_ETH_EVENT_INTR_LSC);
    packetio_port_link_log(port_idx);
    return (0);
}

int packetio_port_configure_lsc_callback(uint16_t port_idx)
{
    return (rte_eth_dev_callback_register(port_idx, RTE_ETH_EVENT_INTR_LSC,
                                          lsc_interrupt_callback, NULL));
}

int32_t packetio_port_drops(uint16_t port_idx)
{
    struct rte_eth_stats stats = {};
    return (rte_eth_stats_get(port_idx, &stats) == 0 ? (stats.imissed & 0x7fffffff) : 0);
}
