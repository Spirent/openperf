#include <cerrno>
#include <sys/eventfd.h>

#include "socket/event_queue_consumer.h"

namespace openperf {
namespace socket {

template <typename Derived>
Derived& event_queue_consumer<Derived>::derived()
{
    return (static_cast<Derived&>(*this));
}

template <typename Derived>
const Derived& event_queue_consumer<Derived>::derived() const
{
    return (static_cast<const Derived&>(*this));
}

template <typename Derived>
int event_queue_consumer<Derived>::fd() const
{
    return (derived().consumer_fd());
}

template <typename Derived>
std::atomic_uint64_t& event_queue_consumer<Derived>::read_idx()
{
    return (derived().ack_read_idx());
}

template <typename Derived>
const std::atomic_uint64_t& event_queue_consumer<Derived>::read_idx() const
{
    return (derived().ack_read_idx());
}

template <typename Derived>
std::atomic_uint64_t& event_queue_consumer<Derived>::write_idx()
{
    return (derived().ack_write_idx());
}

template <typename Derived>
const std::atomic_uint64_t& event_queue_consumer<Derived>::write_idx() const
{
    return (derived().ack_write_idx());
}

template <typename Derived>
uint64_t event_queue_consumer<Derived>::load_read() const
{
    return (read_idx().load(std::memory_order_acquire));
}

template <typename Derived>
uint64_t event_queue_consumer<Derived>::load_write() const
{
    return (write_idx().load(std::memory_order_acquire));
}

template <typename Derived>
void event_queue_consumer<Derived>::add_read(uint64_t add)
{
    read_idx().fetch_add(add, std::memory_order_release);
}

template <typename Derived>
void event_queue_consumer<Derived>::sub_read(uint64_t sub)
{
    read_idx().fetch_sub(sub, std::memory_order_release);
}

template <typename Derived>
uint64_t event_queue_consumer<Derived>::count() const
{
    return (load_write() - load_read());
}

template <typename Derived>
int event_queue_consumer<Derived>::ack()
{
    return (count() ? ack_wait() : 0);
}

template <typename Derived>
int event_queue_consumer<Derived>::ack_wait()
{
    uint64_t counter = 0;
    auto error = eventfd_read(fd(), &counter);
    if (error) return (errno);
    add_read(counter);
    return (0);
}

template <typename Derived>
int event_queue_consumer<Derived>::ack_undo()
{
    if (count()) return (0);

    sub_read(1);
    if (auto error = eventfd_write(fd(), 1); error != 0) {
        add_read(1);
        return (errno);
    }

    return (0);
}

template <typename Derived>
int event_queue_consumer<Derived>::block()
{
    /*
     * If the difference between write and read is eventfd_max, then the
     * fd is already blocked.
     */
    auto events = count();
    if (events == eventfd_max) {
        return (EWOULDBLOCK);
    }

    /*
     * In order to block the fd, we need to write a value, x, such that
     * x + the existing fd value == eventfd_max.
     */
    auto offset = eventfd_max - events;
    sub_read(offset); /* because our counter is a ring */
    if (auto error = eventfd_write(fd(), offset); error != 0) {
        add_read(offset);
        return (errno);
    }
    return (0);
}

template <typename Derived>
int event_queue_consumer<Derived>::block_wait()
{
    /*
     * We might need two writes to block on the fd because we can write at
     * most eventfd_max to the counter, but need eventfd_max + 1 to block.
     * Use a write to take us to the block threshold, if necessary.
     */
    auto offset = eventfd_max - count();
    if (offset) {
        sub_read(offset);
        if (auto error = eventfd_write(fd(), offset); error != 0) {
            add_read(offset);  /* undo previous sub */
            return (errno);
        }
    }

    /* This write should block */
    if (auto error = eventfd_write(fd(), 1); error != 0) {
        return (errno);
    }
    sub_read(1);  /* Don't forget to account for the write! */
    return (0);
}

}
}
