#ifndef _ICP_PACKETIO_STACK_DPDK_STACK_MSG_HANDLER_H_
#define _ICP_PACKETIO_STACK_DPDK_STACK_MSG_HANDLER_H_

#include "lwip/def.h"

#include "packetio/common/dpdk/pollable_event.tcc"

namespace icp::packetio::dpdk {

class stack_msg_handler : public pollable_event<stack_msg_handler>
{
    sys_mbox_t m_mbox;

public:
    stack_msg_handler(sys_mbox_t);

    int event_fd() const;
    uint32_t poll_id() const;

    void handle_event();
};

}

#endif /* _ICP_PACKETIO_STACK_DPDK_STACK_MSG_HANDLER_H_ */
