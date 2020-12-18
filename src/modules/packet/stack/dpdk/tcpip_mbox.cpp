#include "lwip/sys.h"

#include "packet/stack/lwip/tcpip.hpp"
#include "packet/stack/dpdk/tcpip_mbox.hpp"

namespace openperf::packet::stack::dpdk {

static constexpr auto mailbox_size = 128;

sys_mbox_t tcpip_mbox::init()
{
    m_mbox = std::make_unique<sys_mbox>(mailbox_size);
    return (m_mbox.get());
}

void tcpip_mbox::fini() { m_mbox.reset(); }

sys_mbox_t tcpip_mbox::get() { return (m_mbox.get()); }

} // namespace openperf::packet::stack::dpdk

sys_mbox_t openperf::packet::stack::tcpip::mbox()
{
    return (openperf::packet::stack::dpdk::tcpip_mbox::instance().get());
}
