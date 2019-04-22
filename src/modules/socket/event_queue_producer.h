#ifndef _ICP_SOCKET_EVENT_QUEUE_PRODUCER_H_
#define _ICP_SOCKET_EVENT_QUEUE_PRODUCER_H_

#include <atomic>

namespace icp {
namespace socket {

template <typename Derived>
class event_queue_producer
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

}
}

#endif /* _ICP_SOCKET_EVENT_QUEUE_PRODUCER_H_ */
