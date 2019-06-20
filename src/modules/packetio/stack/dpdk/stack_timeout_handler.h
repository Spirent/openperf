#ifndef _ICP_PACKETIO_STACK_DPDK_STACK_TIMEOUT_HANDLER_H_
#define _ICP_PACKETIO_STACK_DPDK_STACK_TIMEOUT_HANDLER_H_

#include "packetio/common/dpdk/pollable_event.tcc"

namespace icp::packetio::dpdk {

class stack_timeout_handler : public pollable_event<stack_timeout_handler>
{
    int m_fd;

public:
    stack_timeout_handler();
    ~stack_timeout_handler();

    uint32_t poll_id() const;

    void handle_event();

    int event_fd() const;
    event_callback event_callback_function() const;
};

}

#endif /* _ICP_PACKETIO_STACK_DPDK_STACK_TIMEOUT_HANDLER_H_ */
