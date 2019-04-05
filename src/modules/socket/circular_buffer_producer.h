#ifndef _ICP_SOCKET_CIRCULAR_BUFFER_PRODUCER_H_
#define _ICP_SOCKET_CIRCULAR_BUFFER_PRODUCER_H_

#include <atomic>
#include <sys/uio.h>

namespace icp {
namespace socket {

template <typename Derived>
class circular_buffer_producer
{
    Derived& derived();
    const Derived& derived() const;

    uint8_t* base() const;
    size_t   len()  const;
    std::atomic_size_t& read_idx();
    const std::atomic_size_t& read_idx() const;
    std::atomic_size_t& write_idx();
    const std::atomic_size_t& write_idx() const;

    size_t load_read() const;
    size_t load_write() const;
    size_t mask(size_t idx) const;

    void store_write(size_t idx);

public:
    size_t writable() const;
    size_t write(const void* ptr, size_t length);

    template <typename NotifyFunction>
    size_t write_and_notify(const void* ptr, size_t length, NotifyFunction&& notify);
};

}
}

#endif /* _ICP_SOCKET_CIRCULAR_BUFFER_PRODUCER_H_ */
