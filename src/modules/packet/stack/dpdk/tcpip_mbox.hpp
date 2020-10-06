#ifndef _OP_PACKET_STACK_DPDK_TCPIP_MBOX_HPP_
#define _OP_PACKET_STACK_DPDK_TCPIP_MBOX_HPP_

#include <memory>

#include "packet/stack/dpdk/sys_mailbox.hpp"
#include "utils/singleton.hpp"

namespace openperf::packet::stack::dpdk {

class tcpip_mbox : public utils::singleton<tcpip_mbox>
{
public:
    sys_mbox_t init();
    void fini();
    sys_mbox_t get();

private:
    std::unique_ptr<sys_mbox> m_mbox;
};

} // namespace openperf::packet::stack::dpdk

#endif /* _OP_PACKET_STACK_DPDK_TCPIP_MBOX_HPP_ */
