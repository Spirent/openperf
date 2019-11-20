#ifndef _OP_SOCKET_CIRCULAR_BUFFER_CONSUMER_HPP_
#define _OP_SOCKET_CIRCULAR_BUFFER_CONSUMER_HPP_

#include <array>
#include <atomic>
#include <sys/uio.h>

namespace openperf {
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
    size_t pread(void* ptr, size_t length, size_t offset);

    template <typename NotifyFunction>
    size_t read_and_notify(void* ptr, size_t length, NotifyFunction&& notify);

    using peek_data = std::array<iovec, 2>;
    peek_data peek() const;

    size_t drop(size_t length);
};

}
}

#endif /* _OP_SOCKET_CIRCULAR_BUFFER_CONSUMER_HPP_ */
