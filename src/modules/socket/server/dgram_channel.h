#ifndef _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_
#define _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_

#include <array>
#include <memory>

#include "socket/server/allocator.h"
#include "socket/circular_buffer_consumer.h"
#include "socket/circular_buffer_producer.h"
#include "socket/event_queue_consumer.h"
#include "socket/event_queue_producer.h"
#include "socket/dgram_channel.h"

struct pbuf;

namespace icp::socket::server {

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

    static constexpr size_t init_buffer_size = 32 * 1024;  /* 64 kb */

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
    dgram_channel(int socket_flags, icp::socket::server::allocator& allocator);
    ~dgram_channel();

    dgram_channel(const dgram_channel&) = delete;
    dgram_channel& operator=(const dgram_channel&&) = delete;

    dgram_channel(dgram_channel&&) = default;
    dgram_channel& operator=(dgram_channel&&) = default;

    int client_fd();
    int server_fd();

    bool send_empty() const;
    bool send(const pbuf *p);
    bool send(const pbuf *p, const dgram_ip_addr* addr, in_port_t);

    bool recv_available() const;
    std::pair<pbuf*, std::optional<dgram_channel_addr>> recv();

    void dump() const;
};

struct dgram_channel_deleter {
    icp::socket::server::allocator* allocator_;
    void operator()(icp::socket::server::dgram_channel *channel) {
        auto allocator = reinterpret_cast<icp::socket::server::allocator*>(
            reinterpret_cast<icp::socket::dgram_channel*>(channel)->allocator);
        channel->~dgram_channel();
        allocator->deallocate(reinterpret_cast<uint8_t*>(channel),
                              sizeof(*channel));
    }
};

typedef std::unique_ptr<dgram_channel, dgram_channel_deleter> dgram_channel_ptr;

}

#endif /* _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_ */
