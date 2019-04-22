#ifndef _ICP_SOCKET_EVENT_QUEUE_CONSUMER_H_
#define _ICP_SOCKET_EVENT_QUEUE_CONSUMER_H_

#include <atomic>

namespace icp {
namespace socket {

template <typename Derived>
class event_queue_consumer
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

    void add_read(uint64_t add);
    void sub_read(uint64_t sub);

    uint64_t count() const;

public:
    int ack();
    int ack_wait();
    int ack_undo();

    int block();
    int block_wait();
};

}
}

#endif /* _ICP_SOCKET_EVENT_QUEUE_CONSUMER_H_ */
