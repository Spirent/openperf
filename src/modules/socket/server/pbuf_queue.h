#ifndef _ICP_SOCKET_SERVER_PBUF_BUFFER_H_
#define _ICP_SOCKET_SERVER_PBUF_BUFFER_H_

#include <deque>
#include <sys/uio.h>

struct pbuf;

namespace icp {
namespace socket {
namespace server {

class pbuf_queue {
    std::deque<pbuf*> m_pbufs;
    size_t m_length;

public:
    pbuf_queue();
    ~pbuf_queue();

    size_t bufs() const;
    size_t length() const;
    size_t iovecs(iovec iovs[], size_t max_iovs) const;

    void push(pbuf*);
    size_t clear(size_t bytes);
};

}
}
}

#endif /* _ICP_SOCKET_SERVER_PBUF_BUFFER_H_ */
