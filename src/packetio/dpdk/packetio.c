#include <sys/types.h>

#include "rte_port.h"

#include "packetio.h"

int packetio_setup(struct rte_mempool *mp)
{
    packetio_mempool_set(mp);
}

const static struct rte_eth_rxmode rx_mode = {
    .max_rx_pkt_len = ETHER_MAX_LEN, /**< Default maximum frame length. */
    .split_hdr_size = 0,
    .header_split   = 0, /**< Header Split disabled. */
    .hw_ip_checksum = 1, /**< IP checksum offload enabled. */
    .hw_vlan_filter = 1, /**< VLAN filtering enabled. */
    .hw_vlan_strip  = 1, /**< VLAN strip enabled. */
    .hw_vlan_extend = 0, /**< Extended VLAN disabled. */
    .jumbo_frame    = 0, /**< Jumbo Frame Support disabled. */
    .hw_strip_crc   = 0, /**< CRC stripping by hardware disabled. */
};

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

static void
_populate_rte_eth_conf(uint8_t port_idx, uint16_t nb_rx_queues, struct rte_eth_conf *conf)
{
    memset(conf, 0, sizeof(*conf));

    conf->rxmode = rx_mode;
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
    conf->intr_conf.lsc = !!icp_dpdk_port_config_lsc_interrupt(port_idx);
    conf->intr_conf.rxq = !!icp_dpdk_port_config_rxq_interrupt(port_idx);
}

static
void _populate_rte_eth_txconf(uint8_t port_idx,
                              struct rte_eth_txconf *txconf)
{
    assert(txconf);

    /* Get the port's capabilities so we can intelligently decide what to do */
    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(port_idx, &info);
    *txconf = info.default_txconf;

    /* We want checksumming, so turn it on if the port supports it. */
    if (info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM) {
        txconf->txq_flags &= ~ETH_TXQ_FLAGS_NOXSUMTCP;
    }

    if (info.tx_offload_capa & DEV_TX_OFFLOAD_UDP_CKSUM) {
        txconf->txq_flags &= ~ETH_TXQ_FLAGS_NOXSUMUDP;
    }

    icp_log(ICP_LOG_INFO, "Using txq flags = 0x%x on port %d\n", txconf->txq_flags, port_idx);
    /* All of our tx code is single threaded, so disable refcounts */
    txconf->txq_flags |= ETH_TXQ_FLAGS_NOREFCOUNT;
}

int packetio_port_start(uint8_t port_idx)
{
    int error = rte_eth_dev_start(port_idx);
    if (error) {
        icp_log(ICP_LOG_ERROR, "Failed to start port %d: %s\n",
                port_idx, rte_strerror(error));
    }
    return (error);
}

/*
 * The DPDK stop function doesn't free mbufs in use by hardware queues.
 * We need to call the tx,rx queue stop functions explicitly to do that.
 */
int packetio_port_stop(uint8_t port_idx, uint16_t nb_rxqs, uint16_t nb_txqs)
{
    if (port_idx >= rte_eth_dev_count()) {
        return (-ENODEV);
    }

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

int packetio_port_setup(uint8_t port_idx,
                        uint16_t nb_rxqs, uint16_t nb_txqs,
                        struct rte_mempool *rx_pool)
{
    /* Make sure port is stopped */
    packetio_port_stop(port_idx, nb_rxqs, nb_txqs);

    /* Configure the port */
    struct rte_eth_conf conf = {};
    _populate_rte_eth_conf(port_idx, nb_rxqs, &conf);

    int error = 0;
    if ((error = rte_eth_dev_configure(port_idx,
                                       nb_rxqs, nb_txqs,
                                       &conf)) != 0) {
        return (error);
    }

    /* Setup TX queue(s) */
    struct rte_eth_txconf txconf = {};
    _populate_rte_eth_txconf(port_idx, &txconf);
    uint16_t nb_tx_desc = packetio_port_config_tx_desc_count(port_idx);
    for (uint16_t q = 0; i < nb_txqs; q++) {
        if ((error = rte_eth_tx_queue_setup(port_idx, q,
                                            nb_tx_desc,
                                            SOCKET_ID_ANY,
                                            &txconf)) != 0) {
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
            return (error);
        }
    }

    /* (re)start the port */
    packetio_port_start(port_idx);

    return (0);
}

/**
 * Just for testing.  Receive and transmit packets on queue 0
 * of the specified port.
 */
int packetio_run(uint8_t port_idx)
{

}
