#include <string>

#include "socket/server/tcp_socket.h"
#include "lwip/tcp.h"
#include "core/op_log.h"

static const char* to_string(enum lwip_event event)
{
    switch (event) {
    case LWIP_EVENT_ACCEPT:
        return ("accept");
    case LWIP_EVENT_SENT:
        return ("sent");
    case LWIP_EVENT_RECV:
        return ("recv");
    case LWIP_EVENT_CONNECTED:
        return ("connected");
    case LWIP_EVENT_POLL:
        return ("poll");
    case LWIP_EVENT_ERR:
        return ("error");
    default:
        return ("umknown");
    }
}

extern "C" {

err_t lwip_tcp_event(void *arg, struct tcp_pcb *pcb, enum lwip_event event,
                     struct pbuf *p, uint16_t size, err_t err)
{
    if (arg == nullptr) {
        /* Nothing to do; our socket has been destroyed */
        return (ERR_RST);
    }

    using tcp_socket = openperf::socket::server::tcp_socket;
    auto socket = reinterpret_cast<tcp_socket*>(arg);

    OP_LOG(OP_LOG_TRACE, "Received tcp %s event: socket = %p, pcb = %p, "
            "pbuf = %p, size = %u, err = %d\n",
            to_string(event), (void*)socket, (void*)pcb, (void*)p, size, err);

    switch (event) {
    case LWIP_EVENT_ACCEPT:
        return (socket->do_lwip_accept(pcb, err));
    case LWIP_EVENT_SENT:
        return (socket->do_lwip_sent(size));
    case LWIP_EVENT_RECV:
        return (socket->do_lwip_recv(p, err));
    case LWIP_EVENT_CONNECTED:
        return (socket->do_lwip_connected(err));
    case LWIP_EVENT_POLL:
        return (socket->do_lwip_poll());
    case LWIP_EVENT_ERR:
        return (socket->do_lwip_error(err));
    default:
        OP_LOG(OP_LOG_WARNING, "Unhandled tcp socket event: %d\n", event);
        return (ERR_VAL);
    }
}

}
