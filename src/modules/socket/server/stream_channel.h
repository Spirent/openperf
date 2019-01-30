#ifndef _ICP_SOCKET_SERVER_STREAM_CHANNEL_H_
#define _ICP_SOCKET_SERVER_STREAM_CHANNEL_H_

#include <memory>

#include "memory/allocator/pool.h"
#include "socket/stream_channel.h"

namespace icp {
namespace socket {
namespace server {

class stream_channel
{
    static constexpr uint64_t eventfd_max = std::numeric_limits<uint64_t>::max() - 1;
    STREAM_CHANNEL_MEMBERS

public:
    stream_channel(int socket_flags);
    ~stream_channel();

    stream_channel(const stream_channel&) = delete;
    stream_channel& operator=(const stream_channel&&) = delete;

    stream_channel(stream_channel&&) = default;
    stream_channel& operator=(stream_channel&&) = default;

    int client_fd();
    int server_fd();

    void ack();
    void error(int);

    void maybe_notify();
    void maybe_unblock();

    size_t send(pid_t pid, const iovec iov[], size_t iovcnt);

    iovec recv_peek();
    size_t recv_clear(size_t length);
};

struct stream_channel_deleter {
    using pool = icp::memory::allocator::pool;
    pool* pool_;
    void operator()(icp::socket::server::stream_channel *c) {
        c->~stream_channel();
        pool_->release(reinterpret_cast<void*>(c));
    }
    stream_channel_deleter(pool* p)
        : pool_(p)
    {}
};

typedef std::unique_ptr<stream_channel, stream_channel_deleter> stream_channel_ptr;

}
}
}

#endif /* _ICP_SOCKET_SERVER_STREAM_CHANNEL_H_ */
