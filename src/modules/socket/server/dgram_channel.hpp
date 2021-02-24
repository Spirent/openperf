#ifndef _OP_SOCKET_SERVER_DGRAM_CHANNEL_HPP_
#define _OP_SOCKET_SERVER_DGRAM_CHANNEL_HPP_

#include <array>
#include <memory>

#include "socket/server/allocator.hpp"
#include "socket/circular_buffer_consumer.hpp"
#include "socket/circular_buffer_producer.hpp"
#include "socket/event_queue_consumer.hpp"
#include "socket/event_queue_producer.hpp"
#include "socket/dgram_channel.hpp"

struct pbuf;

namespace openperf::socket::server {

class dgram_channel
    : public circular_buffer_consumer<dgram_channel>
    , public circular_buffer_producer<dgram_channel>
    , public event_queue_consumer<dgram_channel>
    , public event_queue_producer<dgram_channel>
{
    DGRAM_CHANNEL_MEMBERS

    friend class circular_buffer_consumer<dgram_channel>;
    friend class circular_buffer_producer<dgram_channel>;
    friend class event_queue_consumer<dgram_channel>;
    friend class event_queue_producer<dgram_channel>;

    static constexpr size_t init_buffer_size = 64 * 1024; /* 64 kb */

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
    dgram_channel(int socket_flags,
                  openperf::socket::server::allocator& allocator);
    ~dgram_channel();

    dgram_channel(const dgram_channel&) = delete;
    dgram_channel& operator=(const dgram_channel&&) = delete;

    int client_fd();
    int server_fd();

    bool send_empty() const;
    bool send(pbuf* p);
    bool send(pbuf* p, const dgram_ip_addr* addr, in_port_t);
    bool send(pbuf* p, const dgram_sockaddr_ll* sll);

    bool recv_available() const;

    struct ip_socket_address
    {
        dgram_ip_addr addr;
        in_port_t port;
    };

    std::pair<pbuf*, std::optional<ip_socket_address>> recv_ip();
    std::pair<pbuf*, std::optional<dgram_sockaddr_ll>> recv_ll();

    void dump() const;
};

struct dgram_channel_deleter
{
    openperf::socket::server::allocator* allocator_;
    void operator()(openperf::socket::server::dgram_channel* channel)
    {
        auto allocator = reinterpret_cast<openperf::socket::server::allocator*>(
            reinterpret_cast<openperf::socket::dgram_channel*>(channel)
                ->allocator);
        channel->~dgram_channel();
        allocator->deallocate(reinterpret_cast<uint8_t*>(channel),
                              sizeof(*channel));
    }
};

typedef std::unique_ptr<dgram_channel, dgram_channel_deleter> dgram_channel_ptr;

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_DGRAM_CHANNEL_HPP_ */
