#ifndef _PACKETIO_PORT_H_
#define _PACKETIO_PORT_H_

/**
 * @file
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PACKETIO_MAX_PORTS 16

union mac_address;
struct rte_mempool;

/*
 * Provide a framework for specifying port specific parameters
 */
#define PACKETIO_MAX_PORT_CONFIGS 16
#define PACKETIO_PORT_LSC_INTR (1 << 1)
#define PACKETIO_PORT_RXQ_INTR (1 << 2)

/**
 * ICP/DPDK static port configuration data
 * The contents are used to configure DPDK ports during system initialization
 */
struct packetio_port_config {
    const char *name;       /**< must match DPDK driver name */
    uint16_t nb_rx_queues;  /**< recommended number of rx queues */
    uint16_t nb_tx_queues;  /**< recommended number of tx queues */
    uint16_t nb_rx_desc;    /**< number of rx descriptors */
    uint16_t nb_tx_desc;    /**< number of tx descriptors */
    uint32_t flags;         /**< configuration flags */
};

/**
 * Register a port configuration
 *
 * @param[in] config
 *   port configuration to register
 *
 * @return
 *   -  0: success
 *   - !0: error
 */
int packetio_port_config_register(const struct packetio_port_config *config);

/**
 * Macro to create constructor to register port configurations
 */
#define PACKETIO_PORT_CONFIG_REGISTER(config)             \
    void packetio_port_config_register_##config(void);    \
  __attribute__((constructor))                            \
  void packetio_port_config_register_##config(void)       \
  {                                                       \
      packetio_port_config_register(&config);             \
  }

/**
 * Retrieve a ports suggested rx queue count
 */
uint16_t packetio_port_config_rx_queue_count(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve a ports maximum rx queue count.  This number is limited by the number
 * of available rx threads.
 */
uint16_t packetio_port_config_rx_queue_max(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve a ports recommended tx queue count.
 */
uint16_t packetio_port_config_tx_queue_count(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve a ports maximum tx queue count.  This number is limited by the number
 * of available tx threads.
 */
uint16_t packetio_port_config_tx_queue_max(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve a ports recommended rx descriptor count.
 */
uint16_t packetio_port_config_rx_desc_count(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve a ports recommended tx descriptor count.
 */
uint16_t packetio_port_config_tx_desc_count(uint16_t port_idx) __attribute__((pure));

/**
 * Does this port support LSC interrupts?
 */
bool     packetio_port_config_lsc_interrupt(uint16_t port_idx) __attribute__((pure));

/**
 * Does this port support rx queue interrupts?
 */
bool     packetio_port_config_rxq_interrupt(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve the total number of DPDK ports in the system
 */
uint8_t  packetio_port_count() __attribute__((pure));

/**
 * Retrieve the port's driver name.
 */
const char * packetio_port_driver_name(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve the max number of configurable rx queues on a port.
 */
uint16_t packetio_port_max_rx_queues(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve the max number of configurable tx queues on a port.
 */
uint16_t packetio_port_max_tx_queues(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve the minimum size of rx buffers on a port.
 */
uint32_t packetio_port_min_rx_bufsize(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve the current number of configured rx queues on a port.
 */
uint16_t packetio_port_nb_rx_queues(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve the current number of configured tx queues on a port.
 */
uint16_t packetio_port_nb_tx_queues(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve a ports supported tx offloads.
 */
uint32_t packetio_port_tx_offload_capa(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve the number of dropped frames on a port.
 * This number indicates the number of frames that could not be copied to
 * an rx descriptor due to rx memory pool exhaustion.
 */
int32_t packetio_port_drops(uint16_t port_idx) __attribute__((pure));

/**
 * Retrieve the speed of a port in Mbps.
 */
uint32_t packetio_port_speed(uint16_t port_idx) __attribute__((pure));

/**
 * Set port link to UP on a port
 *
 * @param[in] port_idx
 *   port to enable link
 *
 * @return
 *   -  0: success
 *   - !0: error
 */
int  packetio_port_link_up(uint16_t port_idx);

/**
 * Set port link to DOWN on a port
 *
 * @param[in] port_idx
 *   port to disable link
 *
 * @return
 *   -  0: success
 *   - !0: error
 */
int  packetio_port_link_down(uint16_t port_idx);

/**
 * Check for link on a port
 *
 * @param[in] port_idx
 *   port to check link
 *
 * @return
 *   -  true: link is up
 *   - false: link is down
 */
bool packetio_port_link_check(uint16_t port_idx);

/**
 * Log the current link status on a port
 *
 * @param[in] port_idx
 *   port to log link status
 */
void packetio_port_link_log(uint16_t port_idx);

/**
 * Retrieve the MAC address of a port
 *
 * @param[in] port_idx
 *   the port to query
 * @param[out] mac
 *   the memory area to write the port MAC
 *
 * @return
 *   -  0: success
 *   - !0: error
 */
int packetio_port_mac(uint16_t port_idx, union mac_address *mac);

/**
 * Switch a port to a running state.  A successful return from this function
 * is required for transmitting or receiving any packets.
 *
 * @param[in] port_idx
 *   the port to start
 *
 * @return
 *   -  0: success
 *   - !0: error
 */
int packetio_port_start(uint16_t port_idx);

/**
 * Stop all operations on a port.  After calling this function, transmitting
 * and receiving can not be done until the port is (re)configured.
 *
 * @param[in] port_idx
 *   the port to stop
 *
 * @return
 *   -  0: success
 *   - !0: error
 */
int packetio_port_stop(uint16_t port_idx);

/**
 * Configure a port for frame operations
 *
 * @param[in] port_idx
 *   the port to configure
 * @param[in] nb_rxqs, nb_txqs
 *   the number of rx/tx queues
 * @param[in] rx_pool
 *   the memory pool to use for storing received frames
 *
 * @return
 *   -  0: success
 *   - !0: error
 */
int packetio_port_configure(uint16_t port_idx,
			    uint16_t nb_rxqs, uint16_t nb_txqs,
			    struct rte_mempool *rx_pool);

/**
 * Configure a callback to log link status changes on a port.  The port
 * MUST support the Link Status Callback (LSC) interrupt for this function
 * to succeed.
 *
 * @warning
 *   Calling this function on a port without LSC support will cause a crash
 *
 * @param[in] port_idx
 *   the port to configure with a callback
 *
 * @return
 *   -  0: success
 *   - !0: error
 */
int packetio_port_configure_lsc_callback(uint16_t port_idx);

#endif /* _PACKETIO_PORT_H_ */
