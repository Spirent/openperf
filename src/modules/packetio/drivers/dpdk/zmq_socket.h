#ifndef _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_
#define _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_

#include "packetio/common/dpdk/pollable_event.tcc"

namespace icp::packetio::dpdk {

class zmq_socket : public pollable_event<zmq_socket> {
    int m_fd;
    int m_signal;

public:
    zmq_socket(void* socket);
    ~zmq_socket() = default;

    uint32_t poll_id() const;

    bool readable() const;

    bool enable() const;
    bool disable() const;

    int event_fd() const;
    event_callback event_callback_function() const;
    void* event_callback_argument();
};

}

#endif /* _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_ */
