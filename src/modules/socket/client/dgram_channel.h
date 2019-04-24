#ifndef _ICP_SOCKET_CLIENT_DGRAM_CHANNEL_H_
#define _ICP_SOCKET_CLIENT_DGRAM_CHANNEL_H_

#include "socket/event_queue_consumer.h"
#include "socket/event_queue_producer.h"
#include "socket/dgram_channel.h"

namespace icp {
namespace socket {
namespace client {

class dgram_channel : public event_queue_consumer<dgram_channel>
                    , public event_queue_producer<dgram_channel>
{
    DGRAM_CHANNEL_MEMBERS

    friend class event_queue_consumer<dgram_channel>;
    friend class event_queue_producer<dgram_channel>;

protected:
    int consumer_fd() const;
    int producer_fd() const;

    std::atomic_uint64_t& notify_read_idx();
    const std::atomic_uint64_t& notify_read_idx() const;
    std::atomic_uint64_t& notify_write_idx();
    const std::atomic_uint64_t& notify_write_idx() const;
    std::atomic_uint64_t& ack_read_idx();
    const std::atomic_uint64_t& ack_read_idx() const;
    std::atomic_uint64_t& ack_write_idx();
    const std::atomic_uint64_t& ack_write_idx() const;

public:
    dgram_channel(int client_fd, int server_fd);
    ~dgram_channel();

    dgram_channel(const dgram_channel&) = delete;
    dgram_channel& operator=(const dgram_channel&&) = delete;

    dgram_channel(dgram_channel&&) = default;
    dgram_channel& operator=(dgram_channel&&) = default;

    int error() const;

    int flags() const;
    int flags(int);

    tl::expected<size_t, int> send(pid_t pid,
                                   const iovec iov[], size_t iovcnt,
                                   int flags,
                                   const sockaddr *to);

    tl::expected<size_t, int> recv(pid_t pid,
                                   iovec iov[], size_t iovcnt,
                                   int flags,
                                   sockaddr *from, socklen_t *fromlen);

    tl::expected<void, int> block_writes();
    tl::expected<void, int> wait_readable();
    tl::expected<void, int> wait_writable();
};

}
}
}

#endif /* _ICP_SOCKET_CLIENT_DGRAM_CHANNEL_H_ */
