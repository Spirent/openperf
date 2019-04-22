#include <cassert>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <zmq.h>

#include "packetio/drivers/dpdk/zmq_socket.h"
#include "core/icp_log.h"

namespace icp::packetio::dpdk {

static int get_socket_fd(void *socket)
{
    int fd = -1;
    size_t fd_size = sizeof(fd);

    if (auto error = zmq_getsockopt(socket, ZMQ_FD, &fd, &fd_size); error != 0) {
        return (-1);
    }

    return (fd);
}

zmq_socket::zmq_socket(void* socket)
    : m_fd(get_socket_fd(socket))
{
    if (m_fd == -1) {
        throw std::runtime_error("Could not find fd for socket: "
                                 + std::string(zmq_strerror(errno)));
    }
}

uint32_t zmq_socket::poll_id() const
{
    return (m_fd);
}

bool zmq_socket::readable() const
{
    return (m_signal);
}

static void interrupt_event_callback(int fd __attribute__((unused)), void* arg)
{
    assert(arg);
    auto *signal = reinterpret_cast<int*>(arg);
    *signal = 1;
}

bool zmq_socket::add(int poll_fd, void* data)
{
    m_signal = 0;
    m_event = rte_epoll_event{
        .epdata = {
            .event = EPOLLIN | EPOLLET,
            .data = data,
            .cb_fun = interrupt_event_callback,
            .cb_arg = &m_signal
        }
    };

    auto error = rte_epoll_ctl(poll_fd, EPOLL_CTL_ADD, m_fd, &m_event);
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not add poll event for zmq socket fd %d: %s\n",
                m_fd, strerror(std::abs(error)));
    }

    return (!error);
}

bool zmq_socket::del(int poll_fd, void* data __attribute__((unused)))
{
    auto error = rte_epoll_ctl(poll_fd, EPOLL_CTL_DEL,
                               m_fd, &m_event);

    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not delete poll event for zmq socket %d: %s\n",
                m_fd, strerror(std::abs(error)));
    }

    return (!error);
}

bool zmq_socket::enable() { return (true); }

bool zmq_socket::disable() { return (true); }

}
