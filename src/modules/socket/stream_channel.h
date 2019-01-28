#ifndef _ICP_SOCKET_STREAM_CHANNEL_H_
#define _ICP_SOCKET_STREAM_CHANNEL_H_

#include <atomic>

#include <socket/circular_buffer.h>
#include <socket/spsc_queue.h>
#include <socket/api.h>

namespace icp {
namespace socket {

#define STREAM_CHANNEL_MEMBERS                  \
    circular_buffer sendq;                      \
    circular_buffer recvq;                      \
    api::socket_fd_pair client_fds;             \
    api::socket_fd_pair server_fds;             \
    std::atomic_flag notify_event_flag;         \
    std::atomic_flag block_event_flag;          \
    std::atomic_int socket_error;               \
    std::atomic_int socket_flags;

struct stream_channel {
    STREAM_CHANNEL_MEMBERS
};

}
}

#endif /* _ICP_SOCKET_STREAM_CHANNEL_H_ */
