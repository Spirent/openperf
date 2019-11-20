#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <numeric>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket/circular_buffer_consumer.tcc"
#include "socket/circular_buffer_producer.tcc"
#include "socket/event_queue_consumer.tcc"
#include "socket/event_queue_producer.tcc"

#include "socket/server/stream_channel.hpp"

namespace openperf {
namespace socket {

template class circular_buffer_consumer<server::stream_channel>;
template class circular_buffer_producer<server::stream_channel>;
template class event_queue_consumer<server::stream_channel>;
template class event_queue_producer<server::stream_channel>;

namespace server {

/***
 * protected members required for template derived functionality
 ***/

/* We consume data from the transmit queue */
uint8_t* stream_channel::consumer_base() const
{
    return (tx_buffer.ptr.get());
}

size_t stream_channel::consumer_len() const
{
    return (tx_buffer.len);
}

std::atomic_size_t& stream_channel::consumer_read_idx()
{
    return (tx_q_read_idx);
}

const std::atomic_size_t& stream_channel::consumer_read_idx() const
{
    return (tx_q_read_idx);
}

std::atomic_size_t& stream_channel::consumer_write_idx()
{
    return (tx_q_write_idx);
}

const std::atomic_size_t& stream_channel::consumer_write_idx() const
{
    return (tx_q_write_idx);
}

/* We produce data for the receive queue */
uint8_t* stream_channel::producer_base() const
{
    return (rx_buffer.ptr.get());
}

size_t stream_channel::producer_len() const
{
    return (rx_buffer.len);
}

std::atomic_size_t& stream_channel::producer_read_idx()
{
    return (rx_q_read_idx);
}

const std::atomic_size_t& stream_channel::producer_read_idx() const
{
    return (rx_q_read_idx);
}

std::atomic_size_t& stream_channel::producer_write_idx()
{
    return (rx_q_write_idx);
}

const std::atomic_size_t& stream_channel::producer_write_idx() const
{
    return (rx_q_write_idx);
}

/*
 * We consume notifications on the server fd.
 * We produce notifications on the client fd.
 */
int stream_channel::consumer_fd() const
{
    return (server_fds.server_fd);
}

int stream_channel::producer_fd() const
{
    return (server_fds.client_fd);
}

/* We generate notifications for the receive queue */
std::atomic_uint64_t& stream_channel::notify_read_idx()
{
    return (rx_fd_read_idx);
}

const std::atomic_uint64_t& stream_channel::notify_read_idx() const
{
    return (rx_fd_read_idx);
}

std::atomic_uint64_t& stream_channel::notify_write_idx()
{
    return (rx_fd_write_idx);
}

const std::atomic_uint64_t& stream_channel::notify_write_idx() const
{
    return (rx_fd_write_idx);
}

/* We generate acknowledgements for the transmit queue */
std::atomic_uint64_t& stream_channel::ack_read_idx()
{
    return (tx_fd_read_idx);
}

const std::atomic_uint64_t& stream_channel::ack_read_idx() const
{
    return (tx_fd_read_idx);
}

std::atomic_uint64_t& stream_channel::ack_write_idx()
{
    return (tx_fd_write_idx);
}

const std::atomic_uint64_t& stream_channel::ack_write_idx() const
{
    return (tx_fd_write_idx);
}

/***
 * Public members
 ***/

stream_channel::stream_channel(int flags,
                               openperf::socket::server::allocator& allocator)
    : tx_buffer(allocator.allocate(init_buffer_size), init_buffer_size)
    , tx_q_write_idx(0)
    , rx_q_read_idx(0)
    , tx_fd_write_idx(0)
    , rx_fd_read_idx(0)
    , socket_error(0)
    , socket_flags(0)
    , rx_buffer(allocator.allocate(init_buffer_size), init_buffer_size)
    , tx_q_read_idx(0)
    , rx_q_write_idx(0)
    , tx_fd_read_idx(0)
    , rx_fd_write_idx(0)
    , allocator(&allocator)
{
    /* make sure we can use these interchangeably */
    static_assert(O_NONBLOCK == EFD_NONBLOCK);
    static_assert(O_CLOEXEC == EFD_CLOEXEC);

    /* make sure structure is properly cache aligned */
    assert((reinterpret_cast<uintptr_t>(&tx_buffer) & (socket::cache_line_size - 1)) == 0);
    assert((reinterpret_cast<uintptr_t>(&rx_buffer) & (socket::cache_line_size - 1)) == 0);

    int event_flags = 0;
    if (flags & SOCK_CLOEXEC)  event_flags |= EFD_CLOEXEC;
    if (flags & SOCK_NONBLOCK) event_flags |= EFD_NONBLOCK;

    if ((server_fds.client_fd = eventfd(0, event_flags)) == -1
        || (server_fds.server_fd = eventfd(0, 0)) == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }

    socket_flags.store(event_flags, std::memory_order_release);
}

stream_channel::~stream_channel()
{
    close(server_fds.client_fd);
    close(server_fds.server_fd);

    auto alloc = reinterpret_cast<openperf::socket::server::allocator*>(allocator);
    alloc->deallocate(tx_buffer.ptr.get(), tx_buffer.len);
    alloc->deallocate(rx_buffer.ptr.get(), rx_buffer.len);
}

int stream_channel::client_fd() const
{
    return (server_fds.client_fd);
}

int stream_channel::server_fd() const
{
    return (server_fds.server_fd);
}

void stream_channel::error(int error)
{
    socket_error.store(error, std::memory_order_release);
    notify();
}

size_t stream_channel::send_available() const
{
    return (writable());
}

size_t stream_channel::send_consumable() const
{
    /* e.g. the amount of consumable data in the producer's buffer */
    return (producer_len() - writable());
}

size_t stream_channel::send(const iovec iov[], size_t iovcnt)
{
    auto written = write(iov, iovcnt);

    notify();

    return (written);
}

iovec stream_channel::recv_peek() const
{
    /* XXX: should return both iovec items */
    return (peek()[0]);
}

size_t stream_channel::recv_drop(size_t length)
{
    if (!length) return (0);

    auto adjust = drop(length);
    assert(adjust == length);  /* should always be true for us */

    unblock();

    return (length);
}

void stream_channel::dump() const
{
    fprintf(stderr, "server: tx_q: %zu:%zu, rx_q: %zu:%zu, tx_fd: %zu:%zu, rx_fd: %zu:%zu\n",
            atomic_load(&tx_q_write_idx), atomic_load(&tx_q_read_idx),
            atomic_load(&rx_q_write_idx), atomic_load(&rx_q_read_idx),
            atomic_load(&tx_fd_write_idx), atomic_load(&tx_fd_read_idx),
            atomic_load(&rx_fd_write_idx), atomic_load(&rx_fd_read_idx));
    fflush(stderr);
}

}
}
}
