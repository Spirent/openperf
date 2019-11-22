#include <cassert>
#include <stdexcept>
#include <string>

#include <zmq.h>

#include "packetio/workers/dpdk/zmq_socket.h"
#include "core/op_log.h"

namespace openperf::packetio::dpdk {

static int get_socket_fd(void *socket)
{
    int fd = -1;
    size_t fd_size = sizeof(fd);

    if (zmq_getsockopt(socket, ZMQ_FD, &fd, &fd_size) != 0) {
        return (-1);
    }

    return (fd);
}

zmq_socket::zmq_socket(void* socket, bool* signal)
    : m_socket(socket)
    , m_signal(signal)
    , m_fd(get_socket_fd(socket))
{
    if (m_fd == -1) {
        throw std::runtime_error("Could not acquire fd for socket: "
                                 + std::string(zmq_strerror(errno)));
    }
}

bool zmq_socket::readable() const
{
    uint32_t flags = 0;
    size_t flag_size = sizeof(flags);

    int error = zmq_getsockopt(m_socket, ZMQ_EVENTS, &flags, &flag_size);

    /* XXX: return true if there is an error; let the upper layers figure it out */
    return (error == 0 ? flags & ZMQ_POLLIN : true);
}

int zmq_socket::event_fd() const
{
    return (m_fd);
}

static void interrupt_event_callback(int, void* arg)
{
    assert(arg);
    auto *signal = reinterpret_cast<bool*>(arg);
    *signal = true;
}

pollable_event<zmq_socket>::event_callback zmq_socket::event_callback_function() const
{
    return (interrupt_event_callback);
}

void* zmq_socket::event_callback_argument()
{
    return (reinterpret_cast<void*>(m_signal));
}

}
