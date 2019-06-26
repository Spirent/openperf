#ifndef _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_
#define _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_

#include "packetio/workers/dpdk/pollable_event.tcc"

namespace icp::packetio::dpdk {

class zmq_socket : public pollable_event<zmq_socket> {
    void* m_socket;
    int m_fd;

public:
    zmq_socket(void* socket);
    ~zmq_socket() = default;

    uint32_t poll_id() const;

    bool readable() const;

    int event_fd() const;
};

}

#endif /* _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_ */
