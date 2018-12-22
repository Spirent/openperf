#include "socket/server/tcp_socket.h"
#include "lwip/tcp.h"

namespace icp {
namespace socket {
namespace server {

const char * to_string(const tcp_socket_state& state)
{
    return (std::visit(overloaded_visitor(
                           [](const tcp_init&) -> const char* {
                               return ("init");
                           }),
                       state));
}

tcp_socket::tcp_socket(io_channel* channel)
    : m_pcb(tcp_new())
{}

tcp_socket::~tcp_socket()
{
    /* XXX: this depends on socket state; just abort for now */
    tcp_abort(m_pcb);
}

void tcp_socket::handle_transmit(pid_t pid, io_channel* channel)
{
    (void)pid;
    (void)channel;
}

}
}
}
