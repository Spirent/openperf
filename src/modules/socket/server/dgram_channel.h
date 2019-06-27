#ifndef _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_
#define _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_

#include <array>
#include <memory>

#include "socket/server/allocator.h"
#include "socket/event_queue_consumer.h"
#include "socket/event_queue_producer.h"
#include "socket/dgram_channel.h"

namespace icp {
namespace socket {
namespace server {

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
    dgram_channel(int socket_flags);
    ~dgram_channel();

    dgram_channel(const dgram_channel&) = delete;
    dgram_channel& operator=(const dgram_channel&&) = delete;

    dgram_channel(dgram_channel&&) = default;
    dgram_channel& operator=(dgram_channel&&) = default;

    int client_fd();
    int server_fd();

    bool send_empty() const;
    bool send(const pbuf*);
    bool send(const pbuf*, const dgram_ip_addr*, in_port_t, uint64_t);

    size_t recv(dgram_channel_item items[], size_t max_items);
};

struct dgram_channel_deleter {
    icp::socket::server::allocator* allocator_;
    void operator()(icp::socket::server::dgram_channel *channel) {
        channel->~dgram_channel();
        allocator_->deallocate(reinterpret_cast<uint8_t*>(channel),
                               sizeof(*channel));
    }
    dgram_channel_deleter(icp::socket::server::allocator* allocator)
        : allocator_(allocator)
    {}
};

typedef std::unique_ptr<dgram_channel, dgram_channel_deleter> dgram_channel_ptr;

}
}
}

#endif /* _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_ */
