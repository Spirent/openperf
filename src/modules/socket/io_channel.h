#ifndef _ICP_SOCKET_IO_CHANNEL_H_
#define _ICP_SOCKET_IO_CHANNEL_H_

#include <socket/spsc_queue.h>
#include <socket/api.h>

namespace icp {
namespace socket {

struct pbuf_data {
    uintptr_t pbuf;
    void* ptr;
    size_t size;
};

#define IO_CHANNEL_MEMBERS                                              \
    spsc_queue<pbuf_data,   api::socket_queue_length> recvq;    /* from stack to client */ \
    spsc_queue<iovec,       api::socket_queue_length> sendq;    /* from client to stack */ \
    spsc_queue<uintptr_t,   api::socket_queue_length> returnq;  /* pbufs to be freed in stack */ \
    api::socket_fd_pair client_fds;                                     \
    api::socket_fd_pair server_fds;

struct io_channel {
    IO_CHANNEL_MEMBERS
};

}
}

#endif /* _ICP_SOCKET_IO_CHANNEL_H_ */
