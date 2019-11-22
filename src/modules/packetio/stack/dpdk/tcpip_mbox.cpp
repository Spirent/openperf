#include <cassert>
#include "lwip/sys.h"

#include "packetio/stack/tcpip.hpp"
#include "packetio/stack/dpdk/tcpip_mbox.hpp"

namespace openperf::packetio::dpdk {

sys_mbox_t tcpip_mbox::init()
{
    m_mbox = std::make_unique<sys_mbox>();
    return (m_mbox.get());
}

void tcpip_mbox::fini() { m_mbox.reset(); }

sys_mbox_t tcpip_mbox::get() { return (m_mbox.get()); }

} // namespace openperf::packetio::dpdk

sys_mbox_t openperf::packetio::tcpip::mbox()
{
    return (openperf::packetio::dpdk::tcpip_mbox::instance().get());
}
