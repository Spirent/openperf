#ifndef _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_
#define _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_

#include <array>
#include <memory>

#include "memory/allocator/pool.h"
#include "socket/dgram_channel.h"

struct pbuf;

namespace icp {
namespace socket {
namespace server {

class dgram_channel
{
    static constexpr uint64_t eventfd_max = std::numeric_limits<uint64_t>::max() - 1;
    DGRAM_CHANNEL_MEMBERS

public:
    dgram_channel();
    ~dgram_channel();

    dgram_channel(const dgram_channel&) = delete;
    dgram_channel& operator=(const dgram_channel&&) = delete;

    dgram_channel(dgram_channel&&) = default;
    dgram_channel& operator=(dgram_channel&&) = default;

    int client_fd();
    int server_fd();

    void notify();

    bool send(const pbuf*);
    bool send(const pbuf*, const dgram_ip_addr*, in_port_t);
    void send_wait();

    struct recv_reply {
        size_t nb_items;
        std::optional<dgram_channel_addr> dest;
    };
    recv_reply recv(std::array<iovec, api::socket_queue_length>&);
    void recv_ack();

    void garbage_collect();
};

struct dgram_channel_deleter {
    using pool = icp::memory::allocator::pool;
    pool* pool_;
    void operator()(icp::socket::server::dgram_channel *c) {
        c->~dgram_channel();
        pool_->release(reinterpret_cast<void*>(c));
    }
    dgram_channel_deleter(pool* p)
        : pool_(p)
    {}
};

typedef std::unique_ptr<dgram_channel, dgram_channel_deleter> dgram_channel_ptr;

}
}
}

#endif /* _ICP_SOCKET_SERVER_DGRAM_CHANNEL_H_ */
