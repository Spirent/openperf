#include <cassert>
#include <cerrno>
#include <system_error>

#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>

#include "lwip/pbuf.h"

#include "socket/circular_buffer_consumer.tcc"
#include "socket/circular_buffer_producer.tcc"
#include "socket/event_queue_consumer.tcc"
#include "socket/event_queue_producer.tcc"
#include "socket/server/dgram_channel.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::socket {

template class circular_buffer_consumer<server::dgram_channel>;
template class circular_buffer_producer<server::dgram_channel>;
template class event_queue_consumer<server::dgram_channel>;
template class event_queue_producer<server::dgram_channel>;

namespace server {

/***
 * protected members required for template derived functionality
 ***/

/* We consume data from the transmit queue */
uint8_t* dgram_channel::consumer_base() const { return (tx_buffer.ptr.get()); }

size_t dgram_channel::consumer_len() const { return (tx_buffer.len); }

std::atomic_size_t& dgram_channel::consumer_read_idx()
{
    return (tx_q_read_idx);
}

const std::atomic_size_t& dgram_channel::consumer_read_idx() const
{
    return (tx_q_read_idx);
}

std::atomic_size_t& dgram_channel::consumer_write_idx()
{
    return (tx_q_write_idx);
}

const std::atomic_size_t& dgram_channel::consumer_write_idx() const
{
    return (tx_q_write_idx);
}

/* We produce data for the receive queue */
uint8_t* dgram_channel::producer_base() const { return (rx_buffer.ptr.get()); }

size_t dgram_channel::producer_len() const { return (rx_buffer.len); }

std::atomic_size_t& dgram_channel::producer_read_idx()
{
    return (rx_q_read_idx);
}

const std::atomic_size_t& dgram_channel::producer_read_idx() const
{
    return (rx_q_read_idx);
}

std::atomic_size_t& dgram_channel::producer_write_idx()
{
    return (rx_q_write_idx);
}

const std::atomic_size_t& dgram_channel::producer_write_idx() const
{
    return (rx_q_write_idx);
}

/*
 * We consume notifications on the server fd.
 * We produce notifications on the client fd.
 */
int dgram_channel::consumer_fd() const { return (server_fds.server_fd); }

int dgram_channel::producer_fd() const { return (server_fds.client_fd); }

/* We generate notifications for the receive queue */
std::atomic_uint64_t& dgram_channel::notify_read_idx()
{
    return (rx_fd_read_idx);
}

const std::atomic_uint64_t& dgram_channel::notify_read_idx() const
{
    return (rx_fd_read_idx);
}

std::atomic_uint64_t& dgram_channel::notify_write_idx()
{
    return (rx_fd_write_idx);
}

const std::atomic_uint64_t& dgram_channel::notify_write_idx() const
{
    return (rx_fd_write_idx);
}

/* We generate acknowledgements for the transmit queue */
std::atomic_uint64_t& dgram_channel::ack_read_idx() { return (tx_fd_read_idx); }

const std::atomic_uint64_t& dgram_channel::ack_read_idx() const
{
    return (tx_fd_read_idx);
}

std::atomic_uint64_t& dgram_channel::ack_write_idx()
{
    return (tx_fd_write_idx);
}

const std::atomic_uint64_t& dgram_channel::ack_write_idx() const
{
    return (tx_fd_write_idx);
}

/***
 * Public members
 ***/

dgram_channel::dgram_channel(int flags,
                             openperf::socket::server::allocator& allocator)
    : tx_buffer(allocator.allocate(init_buffer_size), init_buffer_size)
    , tx_q_write_idx(0)
    , rx_q_read_idx(0)
    , tx_fd_write_idx(0)
    , rx_fd_read_idx(0)
    , socket_flags(0)
    , rx_buffer(allocator.allocate(init_buffer_size), init_buffer_size)
    , tx_q_read_idx(0)
    , rx_q_write_idx(0)
    , tx_fd_read_idx(0)
    , rx_fd_write_idx(0)
    , allocator(std::addressof(allocator))
{
    /* make sure we can use these interchangeably */
    static_assert(O_NONBLOCK == EFD_NONBLOCK);
    static_assert(O_CLOEXEC == EFD_CLOEXEC);

    /* make sure structure is properly cache aligned */
    assert((reinterpret_cast<uintptr_t>(std::addressof(tx_buffer))
            & (socket::cache_line_size - 1))
           == 0);
    assert((reinterpret_cast<uintptr_t>(std::addressof(rx_buffer))
            & (socket::cache_line_size - 1))
           == 0);

    int event_flags = 0;
    if (flags & SOCK_CLOEXEC) event_flags |= EFD_CLOEXEC;
    if (flags & SOCK_NONBLOCK) event_flags |= EFD_NONBLOCK;
    socket_flags.store(event_flags, std::memory_order_release);

    if ((server_fds.client_fd = eventfd(0, event_flags)) == -1
        || (server_fds.server_fd = eventfd(0, 0)) == -1) {
        throw std::system_error(
            errno, std::generic_category(), "Could not create eventfd");
    }
}

dgram_channel::~dgram_channel()
{
    close(server_fds.client_fd);
    close(server_fds.server_fd);

    auto alloc =
        reinterpret_cast<openperf::socket::server::allocator*>(allocator);
    alloc->deallocate(tx_buffer.ptr.get(), tx_buffer.len);
    alloc->deallocate(rx_buffer.ptr.get(), rx_buffer.len);
}

int dgram_channel::client_fd() { return (server_fds.client_fd); }

int dgram_channel::server_fd() { return (server_fds.server_fd); }

bool dgram_channel::send_empty() const { return (!readable()); }

static size_t buffer_required(size_t length)
{
    return (sizeof(dgram_channel_descriptor) + length);
}

bool dgram_channel::send(pbuf* p)
{
    if (writable() < buffer_required(p->len)) return (false);

    auto desc = dgram_channel_descriptor{
        .address = std::nullopt,
        .length = p->len,
    };

    auto items = std::array<iovec, 2>{
        iovec{
            .iov_base = std::addressof(desc),
            .iov_len = sizeof(desc),
        },
        iovec{
            .iov_base = p->payload,
            .iov_len = p->len,
        },
    };

    [[maybe_unused]] auto written = write(items.data(), items.size());
    assert(written == buffer_required(p->len));
    notify();
    pbuf_free(p);

    return (true);
}

/*
 * Sanity check our assumptions about the way IPv6 addresses
 * are stored in sockaddr_in6 structures. We need to copy that
 * data out.
 */
static constexpr auto ipv6_addr_octets = 16;
static_assert(
    std::is_same_v<std::remove_reference_t<decltype(
                       std::declval<sockaddr_in6>().sin6_addr.s6_addr)>,
                   uint8_t[ipv6_addr_octets]>);

std::optional<dgram_channel_addr>
to_dgram_channel_addr(const dgram_ip_addr* addr, in_port_t port)
{
    switch (addr->type) {
    case dgram_ip_addr::type::IPv4:
        return (sockaddr_in{.sin_family = AF_INET,
                            .sin_port = port,
                            .sin_addr.s_addr = addr->u_addr.ip4.addr});
    case dgram_ip_addr::type::IPv6: {
        auto sa6 = sockaddr_in6{.sin6_family = AF_INET6,
                                .sin6_port = port,
                                .sin6_scope_id = addr->u_addr.ip6.zone};

        std::copy_n(reinterpret_cast<const uint8_t*>(addr->u_addr.ip6.addr),
                    ipv6_addr_octets,
                    sa6.sin6_addr.s6_addr);
        return (sa6);
    }
    default:
        return (std::nullopt);
    }
}

template <typename Channel, typename SocketAddress>
bool channel_send(Channel& channel, pbuf* p, const SocketAddress& sa)
{
    if (channel.writable() < buffer_required(p->len)) { return (false); }

    auto desc = dgram_channel_descriptor{
        .address = sa,
        .length = p->len,
    };

    auto items = std::array<iovec, 2>{
        iovec{
            .iov_base = std::addressof(desc),
            .iov_len = sizeof(desc),
        },
        iovec{
            .iov_base = p->payload,
            .iov_len = p->len,
        },
    };

    [[maybe_unused]] auto written = channel.write(items.data(), items.size());
    assert(written == buffer_required(p->len));
    channel.notify();
    pbuf_free(p);

    return (true);
}

bool dgram_channel::send(pbuf* p, const dgram_ip_addr* addr, in_port_t port)
{
    return (channel_send(*this, p, to_dgram_channel_addr(addr, port)));
}

bool dgram_channel::send(pbuf* p, const dgram_sockaddr_ll* sll)
{
    return (channel_send(*this, p, *sll));
}

static dgram_channel_descriptor* to_dgram_descriptor(
    const circular_buffer_consumer<dgram_channel>::peek_data&& peek,
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

bool dgram_channel::recv_available() const
{
    auto storage = dgram_channel_descriptor{};
    return (to_dgram_descriptor(peek(), storage) != nullptr);
}

template <typename SocketAddress>
std::enable_if_t<
    std::is_same_v<SocketAddress, dgram_channel::ip_socket_address>,
    dgram_channel::ip_socket_address>
get_socket_address(const dgram_channel_addr& addr)
{
    if (std::holds_alternative<sockaddr_in>(addr)) {
        auto& sin = std::get<sockaddr_in>(addr);
        auto dgram_addr = dgram_ip_addr{
            .u_addr.ip4.addr = sin.sin_addr.s_addr,
            .type = dgram_ip_addr::type::IPv4,
        };
        return (dgram_channel::ip_socket_address{.addr = dgram_addr,
                                                 .port = sin.sin_port});
    }

    auto& sin6 = std::get<sockaddr_in6>(addr);
    assert(sin6.sin6_scope_id < std::numeric_limits<uint8_t>::max());
    auto dgram_addr = dgram_ip_addr{
        .u_addr.ip6.zone = static_cast<uint8_t>(sin6.sin6_scope_id),
        .type = dgram_ip_addr::type::IPv6,
    };
    std::copy_n(sin6.sin6_addr.s6_addr,
                ipv6_addr_octets,
                reinterpret_cast<uint8_t*>(dgram_addr.u_addr.ip6.addr));

    return (dgram_channel::ip_socket_address{.addr = dgram_addr,
                                             .port = sin6.sin6_port});
}

template <typename SocketAddress>
std::enable_if_t<std::is_same_v<SocketAddress, dgram_sockaddr_ll>,
                 dgram_sockaddr_ll>
get_socket_address(const dgram_channel_addr& addr)
{
    return (std::get<dgram_sockaddr_ll>(addr));
}

template <typename SocketAddress, typename Channel>
std::pair<pbuf*, std::optional<SocketAddress>> channel_recv(Channel& channel)
{
    /* Try to get a datagram descriptor */
    auto storage = dgram_channel_descriptor{};
    auto desc = to_dgram_descriptor(channel.peek(), storage);
    if (!desc) { return (std::make_pair(nullptr, std::nullopt)); }

    /* Descriptor found; unload the data */
    auto* p_head = pbuf_alloc(PBUF_TRANSPORT, desc->length, PBUF_POOL);
    if (!p_head) { return (std::make_pair(nullptr, std::nullopt)); }

    assert(p_head->tot_len == desc->length);

    /* Copy ring data into pbuf chain */
    auto copied = 0UL;
    auto* p = p_head;
    while (copied < desc->length && p != nullptr) {
        copied += channel.pread(p->payload, p->len, sizeof(*desc) + copied);
        p = p->next;
    }

    /*
     * Copy address data so that we can drop the descriptor + payload
     * before returning.
     */
    auto addr = std::optional<SocketAddress>{};
    if (desc->address) {
        addr = get_socket_address<SocketAddress>(desc->address.value());
    }
    channel.drop(buffer_required(desc->length));
    channel.unblock();

    return (std::make_pair(p_head, addr));
}

std::pair<pbuf*, std::optional<dgram_channel::ip_socket_address>>
dgram_channel::recv_ip()
{
    return (channel_recv<dgram_channel::ip_socket_address>(*this));
}

std::pair<pbuf*, std::optional<dgram_sockaddr_ll>> dgram_channel::recv_ll()
{
    return (channel_recv<dgram_sockaddr_ll>(*this));
}

void dgram_channel::dump() const
{
    fprintf(stderr,
            "server: tx_q: %zu:%zu, rx_q: %zu:%zu, tx_fd: %zu:%zu, rx_fd: "
            "%zu:%zu\n",
            atomic_load(&tx_q_write_idx),
            atomic_load(&tx_q_read_idx),
            atomic_load(&rx_q_write_idx),
            atomic_load(&rx_q_read_idx),
            atomic_load(&tx_fd_write_idx),
            atomic_load(&tx_fd_read_idx),
            atomic_load(&rx_fd_write_idx),
            atomic_load(&rx_fd_read_idx));
    fflush(stderr);
}

} // namespace server
} // namespace openperf::socket
