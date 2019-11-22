#ifndef _OP_PACKETIO_DPDK_TCPIP_MBOX_HPP_
#define _OP_PACKETIO_DPDK_TCPIP_MBOX_HPP_

#include <memory>

#include "packetio/stack/dpdk/sys_mailbox.hpp"

namespace openperf::packetio::dpdk {

template <typename T> class singleton
{
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    singleton(const singleton&) = delete;
    singleton& operator=(const singleton) = delete;

protected:
    singleton(){};
};

class tcpip_mbox : public singleton<tcpip_mbox>
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
