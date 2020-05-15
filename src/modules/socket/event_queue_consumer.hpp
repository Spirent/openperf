#ifndef _OP_SOCKET_EVENT_QUEUE_CONSUMER_HPP_
#define _OP_SOCKET_EVENT_QUEUE_CONSUMER_HPP_

#include <atomic>
#include <limits>

namespace openperf::socket {

template <typename Derived> class event_queue_consumer
{
    static constexpr uint64_t eventfd_max =
        std::numeric_limits<uint64_t>::max() - 1;

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

} // namespace openperf::socket

#endif /* _OP_SOCKET_EVENT_QUEUE_CONSUMER_HPP_ */
