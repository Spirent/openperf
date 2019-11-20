#ifndef _OP_SOCKET_CLIENT_DGRAM_CHANNEL_HPP_
#define _OP_SOCKET_CLIENT_DGRAM_CHANNEL_HPP_

#include "tl/expected.hpp"

#include "socket/circular_buffer_consumer.hpp"
#include "socket/circular_buffer_producer.hpp"
#include "socket/event_queue_consumer.hpp"
#include "socket/event_queue_producer.hpp"
#include "socket/dgram_channel.hpp"

namespace openperf {
namespace socket {
namespace client {

class dgram_channel : public circular_buffer_consumer<dgram_channel>
                    , public circular_buffer_producer<dgram_channel>
                    , public event_queue_consumer<dgram_channel>
                    , public event_queue_producer<dgram_channel>
{
    DGRAM_CHANNEL_MEMBERS

    friend class circular_buffer_consumer<dgram_channel>;
    friend class circular_buffer_producer<dgram_channel>;
    friend class event_queue_consumer<dgram_channel>;
    friend class event_queue_producer<dgram_channel>;

protected:
    uint8_t* producer_base() const;
    size_t   producer_len() const;
    std::atomic_size_t& producer_read_idx();
    const std::atomic_size_t& producer_read_idx() const;
    std::atomic_size_t& producer_write_idx();
    const std::atomic_size_t& producer_write_idx() const;

    uint8_t* consumer_base() const;
    size_t   consumer_len() const;
    std::atomic_size_t& consumer_read_idx();
    const std::atomic_size_t& consumer_read_idx() const;
    std::atomic_size_t& consumer_write_idx();
    const std::atomic_size_t& consumer_write_idx() const;

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

    tl::expected<size_t, int> send(const iovec iov[], size_t iovcnt,
                                   int flags,
                                   const sockaddr *to);

    tl::expected<size_t, int> recv(iovec iov[], size_t iovcnt,
                                   int flags,
                                   sockaddr *from, socklen_t *fromlen);

    tl::expected<void, int> block_writes();
    tl::expected<void, int> wait_readable();
    tl::expected<void, int> wait_writable();
};

}
}
}

#endif /* _OP_SOCKET_CLIENT_DGRAM_CHANNEL_HPP_ */
