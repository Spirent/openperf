#ifndef _ICP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_H_
#define _ICP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_H_

#include "packetio/generic_port.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace model {

class physical_port
{
public:
    physical_port(int id);

    int id() const;

    static std::string kind();

    port::link_status link()   const;
    port::link_speed  speed()  const;
    port::link_duplex duplex() const;

    port::stats_data  stats()  const;

    port::config_data config() const;
    tl::expected<void, std::string> config(const port::config_data&);

private:
    int m_id;
};

}
}
}
}
#endif /* _ICP_PACKETIO_DPDK_MODEL_PHYSICAL_PORT_H_ */
