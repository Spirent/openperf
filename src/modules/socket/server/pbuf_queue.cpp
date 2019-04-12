#include <algorithm>
#include <cassert>

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
    for (auto& item : m_queue) {
        pbuf_free(item.pbuf());
    }
}

void pbuf_queue::push(pbuf* p)
{
    m_length += p->tot_len;

    /*
     * Break the pbuf chain up into individual pbufs, as they are much
     * easier to manage.  Additionally, clear the next ptr, as we want
     * to be able to free individual pbufs as well.
     */
    while (p != nullptr) {
        m_queue.emplace_back(p, p->payload, p->len);
        auto next_p = p->next;
        p->next = nullptr;
        p = next_p;
    }
}

size_t pbuf_queue::bufs() const
{
    return (m_queue.size());
}

size_t pbuf_queue::length() const
{
    return (m_length);
}

size_t pbuf_queue::iovecs(iovec iovs[], size_t max_iovs, size_t max_length) const
{
    size_t iov_idx = 0, pbuf_idx = 0, length = 0;
    while (pbuf_idx < m_queue.size() && iov_idx < max_iovs) {
        auto& item = m_queue[pbuf_idx++];
        const auto len = item.len();
        if (length + len > max_length) break;
        iovs[iov_idx++] = iovec{ .iov_base = item.payload(),
                                 .iov_len = len };
    }

    return (iov_idx);
}

size_t pbuf_queue::clear(size_t length)
{
    assert(length <= m_length);

    size_t cleared = 0, delete_idx = 0;

    while (!m_queue.empty() && length > 0) {
        auto& item = m_queue[delete_idx++];
        auto len = item.len();
        if (len <= length) {
            cleared += len;
            length -= len;
        } else {
            /*
             * Adjust the pbuf so that the payload points to new data the
             * next time we generate iovecs.
             */
            assert(len > length);

            item.payload(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(item.payload()) + length));
            item.len(len - length);

            cleared += length;
            length = 0;

            delete_idx--;  /* we don't want to delete this one */
        }
    }

    std::for_each(begin(m_queue), begin(m_queue) + delete_idx,
                  [](const pbuf_vec& vec) {
                      pbuf_free(vec.pbuf());
                  });
    m_queue.erase(begin(m_queue), begin(m_queue) + delete_idx);

    m_length -= cleared;

    return (cleared);
}

}
}
}
