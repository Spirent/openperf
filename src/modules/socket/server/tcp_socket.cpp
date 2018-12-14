#include "socket/server/tcp_socket.h"
#include "lwip/tcp.h"

namespace icp {
namespace socket {
namespace server {

tcp_socket::tcp_socket()
    : m_pcb(tcp_new())
{}

tcp_socket::~tcp_socket()
{
    /* XXX: this depends on socket state; just abort for now */
    tcp_abort(m_pcb);
}

}
}
}
