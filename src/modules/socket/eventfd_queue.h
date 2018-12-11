#ifndef _ICP_SOCKET_EVENTFD_QUEUE_H_
#define _ICP_SOCKET_EVENTFD_QUEUE_H_

#include <sys/eventfd.h>

#include "socket/spsc_queue.h"

namespace icp {
namespace sock {

template <typename T, std::size_t N>
struct eventfd_sender {
    spsc_queue<T, N> *m_queue;
    int m_signalfd;
    int m_waitfd;

    eventfd_sender(spsc_queue<T, N>* queue, int signalfd, int waitfd)
        : m_queue(queue)
        , m_signalfd(signalfd)
        , m_waitfd(waitfd)
    {};

    bool send(T& item)
    {
        bool empty = m_queue->empty();
        bool pushed = m_queue->push(item);
        if (pushed && empty) {
            eventfd_write(m_signalfd, 1);
        }
        return (pushed);
    }

    size_t send(T items[], size_t size)
    {
        size_t idx = 0;
        while (idx < size) {
            if (!send(items[idx])) break;
            idx++;
        }
        return (idx++);
    }

    std::pair<int, size_t> wait_status()
    {
        return (std::make_pair(m_waitfd, m_queue->size()));
    }

    void wait()
    {
        uint64_t counter = 0;
        eventfd_read(m_waitfd, &counter);
    }
};

template <typename T, std::size_t N>
struct eventfd_receiver {
    spsc_queue<T, N>* m_queue;
    int m_signalfd;
    int m_waitfd;

    eventfd_receiver(spsc_queue<T, N>* queue, int signalfd, int waitfd)
        : m_queue(queue)
        , m_signalfd(signalfd)
        , m_waitfd(waitfd)
    {}

    std::optional<T> recv()
    {
        bool full = m_queue->full();
        auto item = m_queue->pop();
        if (item && full) {
            eventfd_write(m_signalfd, 1);
        }
        return (item);
    }

    size_t recv(T items[], size_t size)
    {
        size_t idx = 0;
        while (idx < size) {
            auto item = m_queue->pop();
            if (!item) break;
            items[idx++] = item;
        }
        return (idx);
    }

    std::pair<int, size_t> wait_status()
    {
        return (std::make_pair(m_waitfd, m_queue->size()));
    }

    void wait()
    {
        uint64_t counter = 0;
        eventfd_read(m_waitfd, &counter);
    }
};


}
}

#endif /* _ICP_SOCKET_EVENTFD_QUEUE_H_ */
