#ifndef _OP_SOCKET_SERVER_STREAM_CHANNEL_HPP_
#define _OP_SOCKET_SERVER_STREAM_CHANNEL_HPP_

#include <memory>

#include "socket/server/allocator.hpp"
#include "socket/circular_buffer_consumer.hpp"
#include "socket/circular_buffer_producer.hpp"
#include "socket/event_queue_consumer.hpp"
#include "socket/event_queue_producer.hpp"
#include "socket/stream_channel.hpp"

namespace openperf::socket::server {

/*
 * The server side stream channel owns the Tx buffer.
 */
class stream_channel
    : public circular_buffer_consumer<stream_channel>
    , public circular_buffer_producer<stream_channel>
    , public event_queue_consumer<stream_channel>
    , public event_queue_producer<stream_channel>
{
    STREAM_CHANNEL_MEMBERS

    friend class circular_buffer_consumer<stream_channel>;
    friend class circular_buffer_producer<stream_channel>;
    friend class event_queue_consumer<stream_channel>;
    friend class event_queue_producer<stream_channel>;

    static constexpr size_t init_buffer_size = 128 * 1024; /* 128 kb */

protected:
    uint8_t* producer_base() const;
    size_t producer_len() const;
    std::atomic_size_t& producer_read_idx();
    const std::atomic_size_t& producer_read_idx() const;
    std::atomic_size_t& producer_write_idx();
    const std::atomic_size_t& producer_write_idx() const;

    uint8_t* consumer_base() const;
    size_t consumer_len() const;
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
    stream_channel(int socket_flags,
                   openperf::socket::server::allocator& allocator);
    ~stream_channel();

    stream_channel(const stream_channel&) = delete;
    stream_channel& operator=(const stream_channel&&) = delete;

    int client_fd() const;
    int server_fd() const;

    void error(int);

    size_t send_available() const;
    size_t send_consumable() const;
    size_t send(const iovec iov[], size_t iovcnt);

    iovec recv_peek() const;
    size_t recv_drop(size_t length);

    void dump() const;
};

struct stream_channel_deleter
{
    void operator()(openperf::socket::server::stream_channel* channel)
    {
        auto allocator = reinterpret_cast<openperf::socket::server::allocator*>(
            reinterpret_cast<openperf::socket::stream_channel*>(channel)
                ->allocator);
        channel->~stream_channel();
        allocator->deallocate(reinterpret_cast<uint8_t*>(channel),
                              sizeof(*channel));
    }
};

typedef std::unique_ptr<stream_channel, stream_channel_deleter>
    stream_channel_ptr;

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_STREAM_CHANNEL_HPP_ */
