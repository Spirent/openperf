#ifndef _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_
#define _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_

#include "packetio/workers/dpdk/pollable_event.tcc"

namespace icp::packetio::dpdk {

class zmq_socket : public pollable_event<zmq_socket> {
    void* m_socket;
    bool* m_signal;
    int m_fd;

public:
    zmq_socket(void* socket, bool* signal);
    ~zmq_socket() = default;

    bool readable() const;

    int event_fd() const;

    event_callback event_callback_function() const;
    void* event_callback_argument();
};

}

#endif /* _ICP_PACKETIO_DPDK_ZMQ_SOCKET_H_ */
