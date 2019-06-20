#include <limits>

#include "lwip/sys.h"

#include "packetio/stack/dpdk/stack_msg_handler.h"
#include "packetio/stack/tcpip_handlers.h"

namespace icp::packetio::dpdk {

stack_msg_handler::stack_msg_handler(sys_mbox_t mbox)
    : m_mbox(mbox)
{}

uint32_t stack_msg_handler::poll_id() const
{
    return (std::numeric_limits<uint32_t>::max());
}

int stack_msg_handler::event_fd() const
{
    return (sys_mbox_fd(&m_mbox));
}

void stack_msg_handler::handle_event()
{
    tcpip::handle_messages(m_mbox);
}

}
