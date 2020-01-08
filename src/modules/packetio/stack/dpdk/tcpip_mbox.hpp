#ifndef _OP_PACKETIO_DPDK_TCPIP_MBOX_HPP_
#define _OP_PACKETIO_DPDK_TCPIP_MBOX_HPP_

#include <memory>

#include "packetio/stack/dpdk/sys_mailbox.hpp"
#include "utils/singleton.hpp"

namespace openperf::packetio::dpdk {

class tcpip_mbox : public utils::singleton<tcpip_mbox>
{
public:
    sys_mbox_t init();
    void fini();
    sys_mbox_t get();

private:
    std::unique_ptr<sys_mbox> m_mbox;
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_TCPIP_MBOX_HPP_ */
