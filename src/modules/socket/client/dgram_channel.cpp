#include <cassert>
#include <unistd.h>

#include "socket/dpdk/memcpy.hpp"
#include "socket/circular_buffer_consumer.tcc"
#include "socket/circular_buffer_producer.tcc"
#include "socket/event_queue_consumer.tcc"
#include "socket/event_queue_producer.tcc"

#include "socket/client/dgram_channel.hpp"

namespace openperf::socket {

template class circular_buffer_consumer<client::dgram_channel>;
template class circular_buffer_producer<client::dgram_channel>;
template class event_queue_consumer<client::dgram_channel>;
template class event_queue_producer<client::dgram_channel>;

namespace client {

/***
 * protected members required for template derived functionality
 ***/

/* We consume data from the receive queue */
uint8_t* dgram_channel::consumer_base() const
{
    return (rx_buffer.ptr.get());
}

size_t dgram_channel::consumer_len() const
{
    return (rx_buffer.len);
}

std::atomic_size_t& dgram_channel::consumer_read_idx()
{
    return (rx_q_read_idx);
}

const std::atomic_size_t& dgram_channel::consumer_read_idx() const
{
    return (rx_q_read_idx);
}

std::atomic_size_t& dgram_channel::consumer_write_idx()
{
    return (rx_q_write_idx);
}

const std::atomic_size_t& dgram_channel::consumer_write_idx() const
{
    return (rx_q_write_idx);
}

/* We produce data for the transmit queue */
uint8_t* dgram_channel::producer_base() const
{
    return (tx_buffer.ptr.get());
}

size_t dgram_channel::producer_len() const
{
    return (tx_buffer.len);
}

std::atomic_size_t& dgram_channel::producer_read_idx()
{
    return (tx_q_read_idx);
}

const std::atomic_size_t& dgram_channel::producer_read_idx() const
{
    return (tx_q_read_idx);
}

std::atomic_size_t& dgram_channel::producer_write_idx()
{
    return (tx_q_write_idx);
}

const std::atomic_size_t& dgram_channel::producer_write_idx() const
{
    return (tx_q_write_idx);
}

/*
 * We consume notifications on the client fd.
 * We produce notifications on the server fd.
 */
int dgram_channel::consumer_fd() const
{
    return (client_fds.client_fd);
}

int dgram_channel::producer_fd() const
{
    return (client_fds.server_fd);
}

/* We generate notifications for the transmit queue */
std::atomic_uint64_t& dgram_channel::notify_read_idx()
{
    return (tx_fd_read_idx);
}

const std::atomic_uint64_t& dgram_channel::notify_read_idx() const
{
    return (tx_fd_read_idx);
}

std::atomic_uint64_t& dgram_channel::notify_write_idx()
{
    return (tx_fd_write_idx);
}

const std::atomic_uint64_t& dgram_channel::notify_write_idx() const
{
    return (tx_fd_write_idx);
}

/* We generate acknowledgements for the receive queue */
std::atomic_uint64_t& dgram_channel::ack_read_idx()
{
    return (rx_fd_read_idx);
}

const std::atomic_uint64_t& dgram_channel::ack_read_idx() const
{
    return (rx_fd_read_idx);
}

std::atomic_uint64_t& dgram_channel::ack_write_idx()
{
    return (rx_fd_write_idx);
}

const std::atomic_uint64_t& dgram_channel::ack_write_idx() const
{
    return (rx_fd_write_idx);
}

/***
 * Public members
 ***/

static std::optional<dgram_channel_addr> to_addr(const sockaddr *addr)
{
    if (!addr) return (std::nullopt);

    switch (addr->sa_family) {
    case AF_INET: {
        auto sa4 = reinterpret_cast<const sockaddr_in*>(addr);
        return (std::make_optional<dgram_channel_addr>(sa4->sin_addr.s_addr,
                                                       ntohs(sa4->sin_port)));
    }
    case AF_INET6: {
        auto sa6 = reinterpret_cast<const sockaddr_in6*>(addr);
        return (std::make_optional<dgram_channel_addr>(
                    reinterpret_cast<const uint32_t*>(&sa6->sin6_addr.s6_addr),
                    ntohs(sa6->sin6_port)));
    }
    default:
        return (std::nullopt);
    }
}

static std::optional<sockaddr_storage> to_sockaddr(const dgram_channel_addr& addr)
{
    struct sockaddr_storage sstorage;

    switch (addr.addr().type) {
    case dgram_ip_addr::type::IPv4: {
        auto sa4 = reinterpret_cast<sockaddr_in*>(&sstorage);
        sa4->sin_family = AF_INET;
        sa4->sin_port = addr.port();
        sa4->sin_addr.s_addr = addr.addr().u_addr.ip4.addr;
        return (std::make_optional(sstorage));
    }
    case dgram_ip_addr::type::IPv6: {
        auto sa6 = reinterpret_cast<sockaddr_in6*>(&sstorage);
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = addr.port();
        dpdk::memcpy(&sa6->sin6_addr.s6_addr, &addr.addr().u_addr.ip6.addr[0],
                    sizeof(in6_addr));
        return (std::make_optional(sstorage));
    }
    default:
        return (std::nullopt);
    }
}

static socklen_t length_of(const sockaddr_storage& sstorage)
{
    return (sstorage.ss_family == AF_INET
            ? sizeof(sockaddr_in)
            : sizeof(sockaddr_in6));
}

dgram_channel::dgram_channel(int client_fd, int server_fd)
{
    client_fds.client_fd = client_fd;
    client_fds.server_fd = server_fd;
}

dgram_channel::~dgram_channel()
{
    close(client_fds.client_fd);
    close(client_fds.server_fd);

    assert(!readable());
}

int dgram_channel::error() const
{
    /*
     * No async errors for stateless sockets; here for io wrapper
     * compatability.
     */
    return (0);
}

int dgram_channel::flags() const
{
    return (socket_flags.load(std::memory_order_acquire));
}

int dgram_channel::flags(int flags)
{
    socket_flags.store(flags, std::memory_order_release);
    return (0);
}

static size_t buffer_required(size_t length)
{
    return (sizeof(dgram_channel_descriptor) + length);
}

tl::expected<size_t, int> dgram_channel::send(const iovec iov[], size_t iovcnt,
                                              int flags __attribute__((unused)),
                                              const sockaddr *to)
{
    auto iov_length = std::accumulate(iov, iov + iovcnt, 0UL,
                                      [](size_t total, const iovec& iov) {
                                          return (total + iov.iov_len);
                                      });
    if (!iov_length) return (0);  /* success? */

    /*
     * Let the user know if they're trying to send something larger
     * than our channel.
     */
    if (producer_len() < buffer_required(iov_length)) {
        return (tl::make_unexpected(EMSGSIZE));
    }

    size_t buffer_available = 0;
    while ((buffer_available = writable()) < buffer_required(iov_length)) {
        if (auto error = (socket_flags.load(std::memory_order_relaxed) & EFD_NONBLOCK
                          ? block()
                          : block_wait()); error != 0) {
            return (tl::make_unexpected(error));
        }
    }

    assert(buffer_required(iov_length) <= buffer_available);

    /*
     * We have enough buffer space for the message, so generate a header and
     * write it out.
     */
    auto desc = dgram_channel_descriptor{
        .address = to_addr(to),
        .length = static_cast<uint16_t>(iov_length),
    };
    write(std::addressof(desc), sizeof(desc));

    /* Now, write the io vector */
    auto written = write(iov, iovcnt);

    if (buffer_required(written) == buffer_available
        && socket_flags.load(std::memory_order_relaxed)& EFD_NONBLOCK
        && !writable()) {
        block();  /* pre-emptive block */
    }

    notify();

    return (written);
}

static dgram_channel_descriptor*
to_dgram_descriptor(const circular_buffer_consumer<dgram_channel>::peek_data&& peek,
                    struct dgram_channel_descriptor& storage)
{
    auto readable = peek[0].iov_len + peek[1].iov_len;
    if (readable < sizeof(dgram_channel_descriptor)) return (nullptr);

    dgram_channel_descriptor* desc = nullptr;

    if (peek[0].iov_len < sizeof(storage)) {
        /*
         * Our descriptor is split across the ring.  Copy the data into
         * the storage descriptor and update the pointer so that we can
         * return a contiguous descriptor to the caller.
         */
        desc = std::addressof(storage);
        std::copy_n(reinterpret_cast<std::byte*>(peek[0].iov_base),
                    peek[0].iov_len,
                    reinterpret_cast<std::byte*>(desc));
        std::copy_n(reinterpret_cast<std::byte*>(peek[1].iov_base),
                    sizeof(storage) - peek[0].iov_len,
                    reinterpret_cast<std::byte*>(desc) + peek[0].iov_len);
    } else {
        desc = reinterpret_cast<dgram_channel_descriptor*>(peek[0].iov_base);
    }

    assert(desc->tag == descriptor_tag);

    /* Only return a descriptor if the full payload is in the buffer */
    return (readable < buffer_required(desc->length) ? nullptr : desc);
}

tl::expected<size_t, int> dgram_channel::recv(iovec iov[], size_t iovcnt,
                                              int flags,
                                              sockaddr *from, socklen_t *fromlen)
{
    dgram_channel_descriptor* desc = nullptr;
    auto storage = dgram_channel_descriptor{};
    while ((desc = to_dgram_descriptor(peek(), storage)) == nullptr) {
        if (auto error = ack_wait(); error != 0) {
            return (tl::make_unexpected(error));
        }
    }

    assert(desc);

    if (desc->address) {
        auto src = to_sockaddr(desc->address.value());
        if (src && from && fromlen) {
            auto srclen = length_of(src.value());
            dpdk::memcpy(from, &src.value(), std::min(*fromlen, srclen));
            *fromlen = std::max(*fromlen, srclen);
        }
    }

    /*
     * Copy datagram data into our io vector until we run out of data
     * or we run out of vector entries, whichever comes first.
     */
    auto read_size = 0UL;
    auto offset = sizeof(*desc);
    size_t idx = 0;
    while (read_size < desc->length && idx < iovcnt) {
        auto to_read = desc->length - read_size;

        read_size += pread(iov[idx].iov_base,
                           std::min(to_read, iov[idx].iov_len),
                           offset);
        offset += read_size;
        idx++;
    }

    if (!(flags & MSG_PEEK)) {
        /* Drop the whole packet, whether it's read or not */
        drop(buffer_required(desc->length));
    }

    /*
     * Check to see if we have any remaining data to read.  If we don't,
     * clear any pending notification we might have.  Otherwise, make sure
     * a notification remains so we know to come back and read the rest.
     */
    if (!readable()) {
        ack();
    } else {
        ack_undo();
    }

    return (read_size);
}

tl::expected<void, int> dgram_channel::block_writes()
{
    if (auto error = block()) {
        return (tl::make_unexpected(error));
    }
    return {};
}

tl::expected<void, int> dgram_channel::wait_readable()
{
    if (auto error = ack_wait()) {
        return (tl::make_unexpected(error));
    }
    return {};
}

tl::expected<void, int> dgram_channel::wait_writable()
{
    if (auto error = block_wait()) {
        return (tl::make_unexpected(error));
    }
    return {};
}

}
}
