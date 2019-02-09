#ifndef _ICP_PACKETIO_DPDK_MODEL_PORT_INFO_H_
#define _ICP_PACKETIO_DPDK_MODEL_PORT_INFO_H_

#include <cstdint>
#include "packetio/drivers/dpdk/dpdk.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace model {

class port_info
{
public:
    port_info(uint16_t port_id);

    uint16_t id()             const;

    const char* driver_name() const;
    unsigned if_index()       const;

    uint32_t speeds()         const;
    uint32_t max_speed()      const;
    uint32_t max_rx_pktlen()  const;

    uint64_t rx_offloads()    const;
    uint64_t tx_offloads()    const;
    uint64_t rss_offloads()   const;

    uint16_t rx_queue_count() const;
    uint16_t rx_queue_max()   const;
    uint16_t rx_queue_default() const;

    uint16_t tx_queue_count() const;
    uint16_t tx_queue_max()   const;
    uint16_t tx_queue_default() const;

    uint16_t rx_desc_count()  const;
    uint16_t tx_desc_count()  const;

    rte_eth_rxconf default_rxconf() const;
    rte_eth_txconf default_txconf() const;

    uint16_t tx_tso_segment_max() const;

    bool     lsc_interrupt()  const;
    bool     rxq_interrupt()  const;

private:
    const uint16_t m_id;
};

}
}
}
}

#endif /* _ICP_PACKETIO_DPDK_MODEL_PORT_INFO_H_ */
