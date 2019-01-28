#include <stdio.h>

#include "lwip/pbuf.h"
#include "socket/server/pbuf_queue.h"

namespace icp {
namespace socket {
namespace server {

pbuf_queue::pbuf_queue()
    : m_length(0)
{}

pbuf_queue::~pbuf_queue()
{
    while (!m_pbufs.empty()) {
        auto p = m_pbufs.front();
        m_pbufs.pop_front();
        pbuf_free(p);
    }
}

void pbuf_queue::push(pbuf* p)
{
    m_length += p->tot_len;
    m_pbufs.push_back(p);
}

size_t pbuf_queue::bufs() const
{
    return (m_pbufs.size());
}

size_t pbuf_queue::length() const
{
    return (m_length);
}

size_t pbuf_queue::iovecs(iovec iovs[], size_t max_iovs) const
{
    if (m_pbufs.empty()) return (0);

    size_t iov_idx = 0, pbuf_idx = 0;
    auto p = m_pbufs[pbuf_idx++];
    while (p != nullptr && iov_idx < max_iovs) {
        iovs[iov_idx++] = iovec{ .iov_base = p->payload,
                                 .iov_len = p->len };

        /*
         * We have two sources of potential mbufs:
         * the next pbuf in the chain
         * -- or --
         * the next pbuf in the queue
         */
        p = (p->next != nullptr ? p->next
             : pbuf_idx < m_pbufs.size() ? m_pbufs[pbuf_idx++]
             : nullptr);
    }

    return (iov_idx);
}

size_t pbuf_queue::clear(size_t length)
{
    assert(length <= m_length);

    size_t cleared = 0;

    while (!m_pbufs.empty() && length > 0) {
        auto p = m_pbufs.front();
        m_pbufs.pop_front();
        if (p->tot_len <= length) {
            /* easy case */
            cleared += p->tot_len;
            length -= p->tot_len;
            pbuf_free(p);
        } else {
            /*
             * Hard case...
             * Adjust the pbuf so that the payload points to new data
             * the next time we generate iovecs.
             */
            assert(p->tot_len > length);
            while (length > 0) {
                assert(p != nullptr);
                if (length < p->len) {
                    /* new payload is in this pbuf, adjust payload */
                    p->payload = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(p->payload) + length);
                    cleared += length;
                    p->tot_len -= length;
                    p->len -= length;
                    length = 0;
                    m_pbufs.push_front(p);
                } else {
                    /* new payload is somewhere in the chain... */
                    assert(p->next);
                    cleared += p->len;
                    length -= p->len;
                    auto newp = p->tot_len != p->len ? p->next : nullptr;
                    p->next = nullptr;
                    pbuf_free(p);
                    p = newp;
                }
            }
        }
    }

    m_length -= cleared;

    return (cleared);
}

}
}
}
