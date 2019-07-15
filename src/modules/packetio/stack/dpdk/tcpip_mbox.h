#ifndef _ICP_PACKETIO_DPDK_TCPIP_MBOX_H_
#define _ICP_PACKETIO_DPDK_TCPIP_MBOX_H_

#include <memory>

#include "packetio/stack/dpdk/sys_mailbox.h"

namespace icp::packetio::dpdk {

template <typename T>
class singleton {
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    singleton(const singleton&) = delete;
    singleton& operator= (const singleton) = delete;
protected:
    singleton() {};
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

}

#endif /* _ICP_PACKETIO_DPDK_TCPIP_MBOX_H_ */
