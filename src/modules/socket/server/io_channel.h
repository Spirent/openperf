#ifndef _ICP_SOCKET_SERVER_IO_CHANNEL_H_
#define _ICP_SOCKET_SERVER_IO_CHANNEL_H_

#include <array>
#include "socket/io_channel.h"

struct pbuf;

namespace icp {
namespace socket {
namespace server {

class io_channel
{
    static constexpr uint64_t eventfd_max = std::numeric_limits<uint64_t>::max() - 1;
    IO_CHANNEL_MEMBERS

public:
    io_channel();
    ~io_channel();

    io_channel(const io_channel&) = delete;
    io_channel& operator=(const io_channel&&) = delete;

    io_channel(io_channel&&) = default;
    io_channel& operator=(io_channel&&) = default;

    int client_fd();
    int server_fd();

    bool send(const pbuf*);
    bool send(const pbuf*, const io_ip_addr*, in_port_t);
    void send_wait();

    struct recv_reply {
        size_t nb_items;
        std::optional<io_channel_addr> dest;
    };
    recv_reply recv(std::array<iovec, api::socket_queue_length>&);
    void recv_ack();

    void garbage_collect();
};

}
}
}

#endif /* _ICP_SOCKET_SERVER_IO_CHANNEL_H_ */
