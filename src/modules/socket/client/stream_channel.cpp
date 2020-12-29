#include <cassert>
#include <cerrno>
#include <numeric>
#include <unistd.h>

#include "socket/circular_buffer_consumer.tcc"
#include "socket/circular_buffer_producer.tcc"
#include "socket/event_queue_consumer.tcc"
#include "socket/event_queue_producer.tcc"

#include "socket/client/stream_channel.hpp"

namespace openperf::socket {

template class circular_buffer_consumer<client::stream_channel>;
template class circular_buffer_producer<client::stream_channel>;
template class event_queue_consumer<client::stream_channel>;
template class event_queue_producer<client::stream_channel>;

namespace client {

/***
 * protected members required for template derived functionality
 ***/

/* We consume data from the receive queue */
uint8_t* stream_channel::consumer_base() const { return (rx_buffer.ptr.get()); }

size_t stream_channel::consumer_len() const { return (rx_buffer.len); }

std::atomic_size_t& stream_channel::consumer_read_idx()
{
    return (rx_q_read_idx);
}

const std::atomic_size_t& stream_channel::consumer_read_idx() const
{
    return (rx_q_read_idx);
}

std::atomic_size_t& stream_channel::consumer_write_idx()
{
    return (rx_q_write_idx);
}

const std::atomic_size_t& stream_channel::consumer_write_idx() const
{
    return (rx_q_write_idx);
}

/* We produce data for the transmit queue */
uint8_t* stream_channel::producer_base() const { return (tx_buffer.ptr.get()); }

size_t stream_channel::producer_len() const { return (tx_buffer.len); }

std::atomic_size_t& stream_channel::producer_read_idx()
{
    return (tx_q_read_idx);
}

const std::atomic_size_t& stream_channel::producer_read_idx() const
{
    return (tx_q_read_idx);
}

std::atomic_size_t& stream_channel::producer_write_idx()
{
    return (tx_q_write_idx);
}

const std::atomic_size_t& stream_channel::producer_write_idx() const
{
    return (tx_q_write_idx);
}

/*
 * We consume notifications on the client fd.
 * We produce notifications on the server fd.
 */
int stream_channel::consumer_fd() const { return (client_fds.client_fd); }

int stream_channel::producer_fd() const { return (client_fds.server_fd); }

/* We generate notifications for the transmit queue */
std::atomic_uint64_t& stream_channel::notify_read_idx()
{
    return (tx_fd_read_idx);
}

const std::atomic_uint64_t& stream_channel::notify_read_idx() const
{
    return (tx_fd_read_idx);
}

std::atomic_uint64_t& stream_channel::notify_write_idx()
{
    return (tx_fd_write_idx);
}

const std::atomic_uint64_t& stream_channel::notify_write_idx() const
{
    return (tx_fd_write_idx);
}

/* We generate acknowledgements for the receive queue */
std::atomic_uint64_t& stream_channel::ack_read_idx()
{
    return (rx_fd_read_idx);
}

const std::atomic_uint64_t& stream_channel::ack_read_idx() const
{
    return (rx_fd_read_idx);
}

std::atomic_uint64_t& stream_channel::ack_write_idx()
{
    return (rx_fd_write_idx);
}

const std::atomic_uint64_t& stream_channel::ack_write_idx() const
{
    return (rx_fd_write_idx);
}

/***
 * Public members
 ***/

stream_channel::stream_channel(int client_fd, int server_fd)
{
    client_fds.client_fd = client_fd;
    client_fds.server_fd = server_fd;
}

stream_channel::~stream_channel()
{
    close(client_fds.client_fd);
    close(client_fds.server_fd);
}

int stream_channel::error() const
{
    return (socket_error.load(std::memory_order_acquire));
}

int stream_channel::flags() const
{
    return (socket_flags.load(std::memory_order_acquire));
}

int stream_channel::flags(int flags)
{
    socket_flags.store(flags, std::memory_order_release);
    return (0);
}

tl::expected<size_t, int> stream_channel::send(const iovec iov[],
                                               size_t iovcnt,
                                               int flags
                                               __attribute__((unused)),
                                               const sockaddr* to
                                               __attribute__((unused)))
{
    if (auto error = socket_error.load(std::memory_order_relaxed); error != 0) {
        return (tl::make_unexpected(error));
    }

    size_t buf_available = 0;
    while ((buf_available = writable()) == 0) {
        if (auto error =
                (socket_flags.load(std::memory_order_relaxed) & EFD_NONBLOCK
                     ? block()
                     : block_wait());
            error != 0) {
            return (tl::make_unexpected(error));
        }

        /* Check if an error woke us up */
        if (auto error = socket_error.load(std::memory_order_acquire);
            error != 0) {
            return (tl::make_unexpected(error));
        }
    }

    assert(buf_available);

    auto written = std::accumulate(
        iov, iov + iovcnt, 0UL, [&](size_t x, const iovec& iov) {
            return (x + write_and_notify(iov.iov_base, iov.iov_len, [&]() {
                        notify();
                    }));
        });

    if (written == buf_available
        && socket_flags.load(std::memory_order_relaxed) & EFD_NONBLOCK
        && !writable()) {
        block(); /* pre-emptive block */
    };

    notify();

    return (written);
}

tl::expected<size_t, int>
stream_channel::recv(iovec iov[],
                     size_t iovcnt,
                     int flags __attribute__((unused)),
                     sockaddr* from __attribute__((unused)),
                     socklen_t* fromlen __attribute__((unused)))
{
    if ((readable() == 0)
        && (socket_flags.load(std::memory_order_relaxed) & EFD_NONBLOCK)) {
        return (tl::make_unexpected(EAGAIN));
    }

    /*
     * Note: we only check for errors if there is nothing to read in the buffer.
     * This ensures clients clear out all data before we report an error.
     */
    size_t buf_readable = 0;
    while ((buf_readable = readable()) == 0) {
        /* Check if an error exists before blocking */
        if (auto error = socket_error.load(std::memory_order_acquire);
            error != 0) {
            if (error == EOF) return (0);
            return (tl::make_unexpected(error));
        }

        /* try to block on the fd */
        if (auto error = ack_wait(); error != 0) {
            return (tl::make_unexpected(error));
        }
    }

    assert(buf_readable);

    auto read_size = std::accumulate(
        iov, iov + iovcnt, 0UL, [&](size_t x, const iovec& iov) {
            return (x + read_and_notify(iov.iov_base, iov.iov_len, [&]() {
                        notify();
                    }));
        });

    /*
     * Check to see if we have any remaining data to read.  If we don't, clear
     * any notification we might have; otherwise make sure a notification
     * remains so we know to come back and read the rest.
     */
    if (!readable()) {
        ack();
    } else {
        ack_undo();
    }

    return (read_size);
}

tl::expected<void, int> stream_channel::block_writes()
{
    if (auto error = block()) { return (tl::make_unexpected(error)); }
    return {};
}

tl::expected<void, int> stream_channel::wait_readable()
{
    if (auto error = ack_wait()) { return (tl::make_unexpected(error)); }
    return {};
}

tl::expected<void, int> stream_channel::wait_writable()
{
    if (auto error = block_wait()) { return (tl::make_unexpected(error)); }
    return {};
}

} // namespace client
} // namespace openperf::socket
