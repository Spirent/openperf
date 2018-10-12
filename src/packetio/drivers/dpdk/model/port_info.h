#ifndef _ICP_PACKETIO_DPDK_MODEL_PORT_INFO_H_
#define _ICP_PACKETIO_DPDK_MODEL_PORT_INFO_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include "core/icp_init_factory.hpp"

namespace icp {
namespace packetio {
namespace dpdk {
namespace model {

enum class port_info_flag {       NONE = 0,
                              LSC_INTR = 0x01,
                              RXQ_INTR = 0x02 };

struct port_info_data : icp::core::init_factory<port_info_data>
{
    port_info_data(Key) {};

    std::string name;
    uint16_t nb_rx_queues;
    uint16_t nb_tx_queues;
    uint16_t nb_rx_desc;
    uint16_t nb_tx_desc;
    port_info_flag flags;
};

class port_info
{
public:
    port_info(const char *name);

    uint16_t rx_queue_count() const __attribute__((const));
    uint16_t rx_queue_max()   const __attribute__((const));
    uint16_t tx_queue_count() const __attribute__((const));
    uint16_t tx_queue_max()   const __attribute__((const));
    uint16_t rx_desc_count()  const __attribute__((const));
    uint16_t tx_desc_count()  const __attribute__((const));
    bool     lsc_interrupt()  const __attribute__((const));
    bool     rxq_interrupt()  const __attribute__((const));

private:
    std::unique_ptr<port_info_data> _data;
};

}
}
}
}

#endif /* _ICP_PACKETIO_DPDK_MODEL_PORT_INFO_H_ */
