#ifndef _ICP_SOCKET_STREAM_CHANNEL_H_
#define _ICP_SOCKET_STREAM_CHANNEL_H_

#include <socket/circular_buffer.h>
#include <socket/spsc_queue.h>
#include <socket/api.h>

struct pbuf;

namespace icp {
namespace socket {

class stream_channel_buf
{
    const pbuf* m_pbuf;
    const void* m_payload;
    uint32_t m_length;

public:
    stream_channel_buf()
        : m_pbuf(nullptr)
        , m_payload(nullptr)
        , m_length(0)
    {}

    stream_channel_buf(const pbuf* pbuf, const void* payload, size_t length)
        : m_pbuf(pbuf)
        , m_payload(payload)
        , m_length(length)
    {}

    const pbuf* pbuf() const { return (m_pbuf); }
    const void* payload() const { return (m_payload); }
    size_t length() const { return (m_length); }
};

#define STREAM_CHANNEL_MEMBERS                  \
    circular_buffer sendq;                      \
    spsc_queue<stream_channel_buf,              \
               api::socket_queue_length> recvq; \
    spsc_queue<const pbuf*,                     \
               api::socket_queue_length> freeq; \
    api::socket_fd_pair client_fds;             \
    api::socket_fd_pair server_fds;             \
    stream_channel_buf partial;

struct stream_channel {
    STREAM_CHANNEL_MEMBERS
};

}
}

#endif /* _ICP_SOCKET_STREAM_CHANNEL_H_ */
