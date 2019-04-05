#include <cerrno>
#include <sys/eventfd.h>

#include "socket/event_queue_producer.h"

namespace icp {
namespace socket {

template <typename Derived>
Derived& event_queue_producer<Derived>::derived()
{
    return (static_cast<Derived&>(*this));
}

template <typename Derived>
const Derived& event_queue_producer<Derived>::derived() const
{
    return (static_cast<const Derived&>(*this));
}

template <typename Derived>
int event_queue_producer<Derived>::fd() const
{
    return (derived().producer_fd());
}

template <typename Derived>
std::atomic_uint64_t& event_queue_producer<Derived>::read_idx()
{
    return (derived().notify_read_idx());
}

template <typename Derived>
const std::atomic_uint64_t& event_queue_producer<Derived>::read_idx() const
{
    return (derived().notify_read_idx());
}

template <typename Derived>
std::atomic_uint64_t& event_queue_producer<Derived>::write_idx()
{
    return (derived().notify_write_idx());
}

template <typename Derived>
const std::atomic_uint64_t& event_queue_producer<Derived>::write_idx() const
{
    return (derived().notify_write_idx());
}

template <typename Derived>
uint64_t event_queue_producer<Derived>::load_read() const
{
    return (read_idx().load(std::memory_order_acquire));
}

template <typename Derived>
uint64_t event_queue_producer<Derived>::load_write() const
{
    return (write_idx().load(std::memory_order_acquire));
}

template <typename Derived>
uint64_t event_queue_producer<Derived>::count() const
{
    return (load_write() - load_read());
}

template <typename Derived>
int event_queue_producer<Derived>::notify()
{
    auto events = count();
    if (!events) {
        write_idx().fetch_add(1, std::memory_order_release);
        if (auto error = eventfd_write(fd(), 1); error != 0) {
            write_idx().fetch_sub(1, std::memory_order_release);
            return (errno);
        }
    }
    return (0);
}

template <typename Derived>
int event_queue_producer<Derived>::unblock()
{
    bool blocked = (load_write() < load_read());
    if (blocked) {
        uint64_t counter = 0;
        auto error = eventfd_read(fd(), &counter);
        if (error) return (errno);
        write_idx().fetch_sub(counter, std::memory_order_release);
    }
    return (0);
}

}
}
