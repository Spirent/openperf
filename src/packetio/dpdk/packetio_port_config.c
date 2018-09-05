#include <assert.h>
#include <string.h>

#include "core/icp_core.h"
#include "packetio_port.h"

static const struct packetio_port_config *_configs[PACKETIO_MAX_PORT_CONFIGS] = {};

int packetio_port_config_register(const struct packetio_port_config *config)
{
    static size_t idx = 0;
    assert(idx < PACKETIO_MAX_PORT_CONFIGS);
    _configs[idx++] = config;
    return (0);
}

const struct packetio_port_config * _get_port_config(uint16_t port_idx)
{
    const char *name = packetio_port_driver_name(port_idx);
    for (size_t i = 0; _configs[i] != NULL; i++) {
        if (strcmp(name, _configs[i]->name) == 0) {
            return (_configs[i]);
        }
    }

    icp_log(ICP_LOG_WARNING, "No explicit configuration found for port %d (%s)."
            "  Using defaults.\n", port_idx, name);

    return (NULL);
}

__attribute__((weak))
uint16_t packetio_port_config_rx_queue_count(uint16_t port_idx)
{
    const struct packetio_port_config *config = _get_port_config(port_idx);
    return (config ? config->nb_rx_queues : 0xFFFF);
}

uint16_t packetio_port_config_rx_queue_max(uint16_t port_idx)
{
    return (icp_min(packetio_port_max_rx_queues(port_idx), 4U));
}

__attribute__((weak))
uint16_t packetio_port_config_tx_queue_count(uint16_t port_idx)
{
    const struct packetio_port_config *config = _get_port_config(port_idx);
    return (config ? config->nb_tx_queues : 0xFFFF);
}

uint16_t packetio_port_config_tx_queue_max(uint16_t port_idx)
{
    return (packetio_port_max_tx_queues(port_idx));
}

uint16_t packetio_port_config_rx_desc_count(uint16_t port_idx)
{
    const struct packetio_port_config *config = _get_port_config(port_idx);
    return (config ? config->nb_rx_desc : 1024);
}

uint16_t packetio_port_config_tx_desc_count(uint16_t port_idx)
{
    const struct packetio_port_config *config = _get_port_config(port_idx);
    return (config ? config->nb_tx_desc : 256);
}

bool packetio_port_config_lsc_interrupt(uint16_t port_idx)
{
    const struct packetio_port_config *config = _get_port_config(port_idx);
    return (config ? config->flags & PACKETIO_PORT_LSC_INTR : false);
}

bool packetio_port_config_rxq_interrupt(uint16_t port_idx)
{
    const struct packetio_port_config *config = _get_port_config(port_idx);
    return (config ? config->flags & PACKETIO_PORT_RXQ_INTR : false);
}

/* Pure virtual drivers */
static const struct packetio_port_config af_packet_config = {
    .name = "net_af_packet",
    .nb_rx_queues = 0xFFFF,
    .nb_tx_queues = 0xFFFF,
    .nb_rx_desc = 0,
    .nb_tx_desc = 0,
    .flags = 0,
};

static const struct packetio_port_config ring_config = {
    .name = "net_ring",
    .nb_rx_queues = 1,
    .nb_tx_queues = 1,
    .nb_rx_desc = 0,
    .nb_tx_desc = 0,
    .flags = 0,
};

/* Common virtual guest drivers */
static const struct packetio_port_config virtio_config = {
    .name = "net_virtio",
    .nb_rx_queues = 1,
    .nb_tx_queues = 1,
    .nb_rx_desc = 256,
    .nb_tx_desc = 256,
    .flags = PACKETIO_PORT_LSC_INTR | PACKETIO_PORT_RXQ_INTR,
};

static const struct packetio_port_config vmxnet3_config = {
    .name = "net_vmxnet3",
    .nb_rx_queues = 1,
    .nb_tx_queues = 1,
    .nb_rx_desc = 2048,
    .nb_tx_desc = 512,
    .flags = 0,
};

/* Various hardware drivers */
static const struct packetio_port_config em_config = {
    .name = "net_e1000_em",
    .nb_rx_queues = 1,
    .nb_tx_queues = 1,
    .nb_rx_desc = 1024,
    .nb_tx_desc = 256,
    .flags = 0,
};

static const struct packetio_port_config igb_config = {
    .name = "net_e1000_igb",
    .nb_rx_queues = 1,
    .nb_tx_queues = 1,
    .nb_rx_desc = 1024,
    .nb_tx_desc = 256,
    .flags = PACKETIO_PORT_LSC_INTR | PACKETIO_PORT_RXQ_INTR,
};

static const struct packetio_port_config igb_vf_config = {
    .name = "net_igb_vf",
    .nb_rx_queues = 1,
    .nb_tx_queues = 1,
    .nb_rx_desc = 1024,
    .nb_tx_desc = 256,
    .flags = PACKETIO_PORT_RXQ_INTR,
};

static const struct packetio_port_config ixgbe_config = {
    .name = "net_ixgbe",
    .nb_rx_queues = 1,
    .nb_tx_queues = 1,
    .nb_rx_desc = 2048,
    .nb_tx_desc = 256,
    .flags = PACKETIO_PORT_LSC_INTR | PACKETIO_PORT_RXQ_INTR,
};

static const struct packetio_port_config ixgbe_vf_config = {
    .name = "net_ixgbe_vf",
    .nb_rx_queues = 1,
    .nb_tx_queues = 1,
    .nb_rx_desc = 2048,
    .nb_tx_desc = 256,
    .flags = PACKETIO_PORT_RXQ_INTR,
};

PACKETIO_PORT_CONFIG_REGISTER(af_packet_config)
PACKETIO_PORT_CONFIG_REGISTER(ring_config)
PACKETIO_PORT_CONFIG_REGISTER(virtio_config)
PACKETIO_PORT_CONFIG_REGISTER(vmxnet3_config)
PACKETIO_PORT_CONFIG_REGISTER(em_config)
PACKETIO_PORT_CONFIG_REGISTER(igb_config)
PACKETIO_PORT_CONFIG_REGISTER(igb_vf_config)
PACKETIO_PORT_CONFIG_REGISTER(ixgbe_config)
PACKETIO_PORT_CONFIG_REGISTER(ixgbe_vf_config)
