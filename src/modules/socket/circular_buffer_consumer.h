#ifndef _ICP_SOCKET_CIRCULAR_BUFFER_CONSUMER_H_
#define _ICP_SOCKET_CIRCULAR_BUFFER_CONSUMER_H_

#include <atomic>
#include <sys/uio.h>

namespace icp {
namespace socket {

template <typename Derived>
class circular_buffer_consumer
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

    void store_read(size_t idx);

public:
    size_t readable() const;
    size_t read(void* ptr, size_t length);

    template <typename NotifyFunction>
    size_t read_and_notify(void* ptr, size_t length, NotifyFunction&& notify);

    iovec  peek() const;
    size_t drop(size_t length);
};

}
}

#endif /* _ICP_SOCKET_CIRCULAR_BUFFER_CONSUMER_H_ */
