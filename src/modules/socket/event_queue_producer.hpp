#ifndef _OP_SOCKET_EVENT_QUEUE_PRODUCER_HPP_
#define _OP_SOCKET_EVENT_QUEUE_PRODUCER_HPP_

#include <atomic>

namespace openperf::socket {

template <typename Derived> class event_queue_producer
{
    Derived& derived();
    const Derived& derived() const;

    int fd() const;
    std::atomic_uint64_t& read_idx();
    const std::atomic_uint64_t& read_idx() const;
    std::atomic_uint64_t& write_idx();
    const std::atomic_uint64_t& write_idx() const;

    uint64_t load_read() const;
    uint64_t load_write() const;

    uint64_t count() const;

public:
    int notify();
    int unblock();
};

} // namespace openperf::socket

#endif /* _OP_SOCKET_EVENT_QUEUE_PRODUCER_HPP_ */
