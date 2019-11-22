#ifndef _OP_SOCKET_SERVER_PBUF_BUFFER_HPP_
#define _OP_SOCKET_SERVER_PBUF_BUFFER_HPP_

#include <vector>
#include <sys/uio.h>

#include "socket/server/pbuf_vec.hpp"

namespace openperf {
namespace socket {
namespace server {

class pbuf_queue
{
    std::vector<pbuf_vec> m_queue;
    size_t m_length;

public:
    pbuf_queue();
    ~pbuf_queue();

    size_t bufs() const;
    size_t length() const;
    size_t iovecs(iovec iovs[], size_t max_iovs, size_t max_length) const;

    void push(pbuf*);
    size_t clear(size_t bytes);
};

} // namespace server
} // namespace socket
} // namespace openperf

#endif /* _OP_SOCKET_SERVER_PBUF_BUFFER_HPP_ */
