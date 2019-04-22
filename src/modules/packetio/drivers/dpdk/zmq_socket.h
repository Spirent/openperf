#ifndef _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_
#define _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_

#include "packetio/drivers/dpdk/dpdk.h"

namespace icp::packetio::dpdk {

class zmq_socket {
    int m_fd;
    int m_signal;
    struct rte_epoll_event m_event;

public:
    zmq_socket(void* socket);
    ~zmq_socket() = default;

    /* Unmovable, as we share our innards with a DPDK function */
    zmq_socket(zmq_socket&&) = delete;
    zmq_socket& operator=(zmq_socket&&) = delete;

    uint32_t poll_id() const;
    bool readable() const;

    bool add(int poll_fd, void* data);
    bool del(int poll_fd, void* data);

    bool enable();
    bool disable();
};

}

#endif /* _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_ */
