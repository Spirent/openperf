#include <cassert>
#include <cstring>
#include <numeric>

#include "socket/server/compat/linux/tcp.h"
#include "socket/server/lwip_utils.hpp"
#include "socket/server/tcp_socket.hpp"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "utils/overloaded_visitor.hpp"

#include "core/op_log.h"

namespace openperf::socket::server {

static constexpr auto tcp_backlog_max =
    static_cast<int>(std::numeric_limits<uint8_t>::max());

const char* to_string(const tcp_socket_state& state)
{
    return (std::visit(
        utils::overloaded_visitor(
            [](const tcp_init&) -> const char* { return ("init"); },
            [](const tcp_bound&) -> const char* { return ("bound"); },
            [](const tcp_accept&) -> const char* { return ("accept"); },
            [](const tcp_listening&) -> const char* { return ("listening"); },
            [](const tcp_connecting&) -> const char* { return ("connecting"); },
            [](const tcp_connected&) -> const char* { return ("connected"); },
            [](const tcp_closing&) -> const char* { return ("closing"); },
            [](const tcp_closed&) -> const char* { return ("closed"); },
            [](const tcp_error&) -> const char* { return ("error"); }),
        state));
}

#if 0
static std::string to_string(tcp_pcb* pcb)
{
    std::string s;
    switch (pcb->state) {
    case LISTEN:
        s += "LISTEN: local port " + std::to_string(pcb->local_port);
        break;
    case CLOSED:
        s += "CLOSED: local port " + std::to_string(pcb->local_port);
        break;
    default:
        s += (std::string(tcp_debug_state_str(pcb->state))
              + ": local port " + std::to_string(pcb->local_port)
              + " remote port " + std::to_string(pcb->remote_port)
              + " snd_next " + std::to_string(pcb->snd_nxt)
              + " rcv_next " + std::to_string(pcb->rcv_nxt));
    }

    return (s);
}
#endif

/*
 * The BufferEmptyFunction should return true if the written amount equals the
 * buffer's current size.  We use a lambda for this so that we can perform this
 * check at the very end of the transmission process.  If the client makes any
 * write to the buffer before we finish, we don't want to set the PSH flag.
 */
template <typename BufferEmptyFunction>
static size_t do_tcp_transmit(tcp_pcb* pcb,
                              const void* ptr,
                              size_t length,
                              BufferEmptyFunction&& empty)
{
    static constexpr uint16_t tcp_send_max =
        std::numeric_limits<uint16_t>::max();

    /* Don't bother invoking TCP machinery to write nothing. */
    if (length == 0) return (0);

    /* If the send buffer is too small, try to make some room */
    if (tcp_sndbuf(pcb) < length) { tcp_output(pcb); }

    size_t written = 0;
    auto to_write = std::min(length, static_cast<size_t>(pcb->snd_buf));

    while (to_write > tcp_send_max) {
        if (tcp_write(pcb,
                      reinterpret_cast<const uint8_t*>(ptr) + written,
                      tcp_send_max,
                      TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE)
            != ERR_OK) {
            return (written);
        }
        written += tcp_send_max;
        to_write -= tcp_send_max;
    }

    /* We want to set the PSH flag if this next write will empty the buffer */
    const auto push =
        std::forward<BufferEmptyFunction>(empty)(written + to_write);
    const auto flags = TCP_WRITE_FLAG_COPY | (-(!push) & TCP_WRITE_FLAG_MORE);
    if (tcp_write(pcb,
                  reinterpret_cast<const uint8_t*>(ptr) + written,
                  to_write,
                  flags)
        != ERR_OK) {
        return (written);
    }

    written += to_write;

    /* Trigger a transmission if we have a push. */
    if (push) tcp_output(pcb);

    return (written);
}

static void do_tcp_transmit_all(tcp_pcb* pcb, stream_channel& channel)
{
    for (;;) {
        auto iov = channel.recv_peek();
        if (channel.recv_drop(do_tcp_transmit(
                pcb,
                iov.iov_base,
                iov.iov_len,
                [&](size_t written) { return (written == iov.iov_len); }))
            == 0) {
            break;
        }
    }
}

static void do_tcp_receive(tcp_pcb* pcb, size_t length)
{
    static constexpr uint16_t tcp_recv_max =
        std::numeric_limits<uint16_t>::max();
    while (length >= tcp_recv_max) {
        tcp_recved(pcb, tcp_recv_max);
        length -= tcp_recv_max;
    }
    tcp_recved(pcb, length);
}

static void
do_tcp_receive_all(tcp_pcb* pcb, stream_channel& channel, pbuf_queue& queue)
{
    std::array<iovec, 32> iovecs;
    size_t iovcnt = 0, cleared = 0;
    while ((iovcnt = queue.iovecs(
                iovecs.data(), iovecs.size(), channel.send_available()))
               > 0
           && (cleared = queue.clear(channel.send(iovecs.data(), iovcnt)))
                  > 0) {
        do_tcp_receive(pcb, cleared);
    }
}

void tcp_socket::tcp_pcb_deleter::operator()(tcp_pcb* pcb)
{
    /* quick and dirty */
    if (tcp_close(pcb) != ERR_OK) { tcp_abort(pcb); }
}

openperf::socket::server::allocator* tcp_socket::channel_allocator()
{
    return (reinterpret_cast<openperf::socket::server::allocator*>(
        reinterpret_cast<socket::stream_channel*>(m_channel.get())->allocator));
}

tcp_socket::tcp_socket(openperf::socket::server::allocator* allocator,
                       int flags,
                       tcp_pcb* pcb)
    : m_channel(new (allocator->allocate(sizeof(stream_channel)))
                    stream_channel(flags, *allocator))
    , m_pcb(pcb)
{
    ::tcp_arg(m_pcb.get(), this);
    ::tcp_poll(m_pcb.get(), nullptr, 2U);
    state(tcp_connected());
}

tcp_socket::tcp_socket(openperf::socket::server::allocator& allocator,
                       enum lwip_ip_addr_type ip_type,
                       int flags)
    : m_channel(new (allocator.allocate(sizeof(stream_channel)))
                    stream_channel(flags, allocator))
    , m_pcb(tcp_new_ip_type(ip_type))
{
    ::tcp_arg(m_pcb.get(), this);
    ::tcp_poll(m_pcb.get(), nullptr, 2U);
}

tcp_socket::~tcp_socket()
{
    /*
     * Disassociate ourselves from our pcb.  The pcb might still exist in
     * the stack after this point, but we don't want it attempting to use
     * this socket to handle its callbacks...
     */
    if (m_pcb) ::tcp_arg(m_pcb.get(), nullptr);
}

/**
 * Explicit move functions are required to ensure that the lwIP PCB has the
 * correct pointer to our socket object.
 */
tcp_socket& tcp_socket::operator=(tcp_socket&& other) noexcept
{
    if (this != &other) {
        m_channel = std::move(other.m_channel);
        m_pcb = std::move(other.m_pcb);
        m_acceptq = std::move(other.m_acceptq);
        ::tcp_arg(m_pcb.get(), this);
        state(other.state());
    }
    return (*this);
}

tcp_socket::tcp_socket(tcp_socket&& other) noexcept
    : m_channel(std::move(other.m_channel))
    , m_pcb(std::move(other.m_pcb))
    , m_acceptq(std::move(other.m_acceptq))
{
    ::tcp_arg(m_pcb.get(), this);
    state(other.state());
}

channel_variant tcp_socket::channel() const { return (m_channel.get()); }

int tcp_socket::do_lwip_accept(tcp_pcb* newpcb, int err)
{
    if (err != ERR_OK || newpcb == nullptr) { return (ERR_VAL); }

    /*
     * XXX: Clear the argument from the newpcb. lwIP helpfully(?) sets this
     * to the same value as the listening socket. Since our argument points
     * to the c++ socket wrapper, that behavior is very unhelpful for us.
     */
    ::tcp_arg(newpcb, nullptr);
    newpcb->netif_idx = m_pcb->netif_idx;
    m_acceptq.emplace(newpcb);
    tcp_backlog_delayed(newpcb);
    m_channel->notify();

    return (ERR_OK);
}

int tcp_socket::do_lwip_sent(uint16_t size __attribute__((unused)))
{
    /*
     * The stack just cleared some data from the send buffer. See if
     * we can fill it up again.
     */
    do_tcp_transmit_all(m_pcb.get(), *m_channel);

    return (ERR_OK);
}

int tcp_socket::do_lwip_recv(pbuf* p, int err)
{
    if (err != ERR_OK) {
        if (p != nullptr) pbuf_free(p);
        do_lwip_error(err);
        return (ERR_OK);
    }

    if (p == nullptr) {
        state(tcp_closed());
        m_channel->error(EOF);
        return (ERR_OK);
    }

    /* Store the incoming pbuf in our buffer */
    m_recvq.push(p);

    /*
     * Initiate a copy to the client's buffer if the TCP sender sent a
     * PUSH flag or we have more than enough data to fill the client's
     * receive buffer.
     */
    if (p->flags & PBUF_FLAG_PUSH
        || m_recvq.length() > m_channel->send_available()) {
        do_tcp_receive_all(m_pcb.get(), *m_channel, m_recvq);
    }

    return (ERR_OK);
}

int tcp_socket::do_lwip_connected(int err)
{
    if (err != ERR_OK) {
        do_lwip_error(err);
        return (ERR_OK);
    }

    /* transition to the connected state */
    state(tcp_connected());

    /* clients may now write to the socket */
    m_channel->unblock();

    return (ERR_OK);
}

int tcp_socket::do_lwip_poll()
{
    /* need to check idle time and close if necessary */

    /*
     * If the connection stalled due to re-transmits, we might have data to
     * send and/or receive
     */
    if (m_recvq.length()) {
        /* Data in the receive queue; try to pass to client */
        do_tcp_receive_all(m_pcb.get(), *m_channel, m_recvq);
    } else if (m_channel->send_consumable()) {
        /* Client buffer has data; (re)notify */
        OP_LOG(OP_LOG_DEBUG, "Kicking idle client\n");
        m_channel->notify();
    }

    /* Try to transmit any data in our send buffer. */
    do_tcp_transmit_all(m_pcb.get(), *m_channel);

    return (ERR_OK);
}

int tcp_socket::do_lwip_error(int err)
{
    m_channel->error(err_to_errno(err));

    /*
     * If we are in the connecting state, then we need to unblock
     * the client
     */
    if (std::holds_alternative<tcp_connecting>(state())) {
        m_channel->unblock();
    }

    state(tcp_error{.value = err_to_errno(err)});

    /*
     * lwIP will "conveniently" free the pbuf for us before this callback is
     * triggered, so make sure we don't.
     */
    [[maybe_unused]] auto* pcb = m_pcb.release();
    return (ERR_OK);
}

tl::expected<generic_socket, int> tcp_socket::handle_accept(int flags)
{
    if (!std::holds_alternative<tcp_listening>(state())) {
        return (tl::make_unexpected(EINVAL));
    }

    if (m_acceptq.empty()) { return (tl::make_unexpected(EAGAIN)); }

    auto pcb = m_acceptq.front();
    m_acceptq.pop();

    // client will ack the event queue before sending accept request so
    // need to notify again if there are more connections to accept
    if (!m_acceptq.empty()) { m_channel->notify(); }

    tcp_backlog_accepted(pcb);
    try {
        return (generic_socket(tcp_socket(channel_allocator(), flags, pcb)));
    } catch (const std::bad_alloc&) {
        // Unable to allocate a socket channel so close the connection
        std::array<char, IPADDR_STRLEN_MAX> local_ip;
        std::array<char, IPADDR_STRLEN_MAX> remote_ip;
        ipaddr_ntoa_r(&pcb->local_ip, local_ip.data(), local_ip.size());
        ipaddr_ntoa_r(&pcb->remote_ip, remote_ip.data(), remote_ip.size());
        OP_LOG(OP_LOG_DEBUG,
               "Failed to allocate socket.  Closing connection "
               "(local=%s:%hu remote=%s:%hu)",
               local_ip.data(),
               pcb->local_port,
               remote_ip.data(),
               pcb->remote_port);
        if (tcp_close(pcb) != ERR_OK) { tcp_abort(pcb); }
        return tl::make_unexpected(ENOMEM);
    }
}

void tcp_socket::handle_io()
{
    if (!m_pcb) return; /* can be closed on error */

    m_channel->ack();

    do_tcp_receive_all(m_pcb.get(), *m_channel, m_recvq);
    do_tcp_transmit_all(m_pcb.get(), *m_channel);
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_bind& bind, const tcp_init&)
{
    sockaddr_storage sstorage;

    auto copy_result = copy_in(sstorage, bind.id.pid, bind.name, bind.namelen);
    if (!copy_result) {
        return {tl::make_unexpected(copy_result.error()), std::nullopt};
    }

    auto ip_addr = get_address(sstorage, IP_IS_V6_VAL(m_pcb->local_ip));
    auto ip_port = get_port(sstorage);

    if (!ip_addr || !ip_port) {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }

    auto error = tcp_bind(m_pcb.get(), &*ip_addr, *ip_port);
    if (error != ERR_OK) {
        return {tl::make_unexpected(err_to_errno(error)), std::nullopt};
    }

    return {api::reply_success(), tcp_bound()};
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_listen& listen, const tcp_bound&)
{
    auto backlog = listen.backlog;
    if (backlog > tcp_backlog_max) {
        // Clip to max limit (same as Linux does)
        OP_LOG(OP_LOG_INFO,
               "TCP socket listen backlog %d is too large.  "
               "Using maximum supported value %d.",
               backlog,
               tcp_backlog_max);
        backlog = tcp_backlog_max;
    }
    /*
     * The lwIP listen function deallocates the original pcb and replaces it
     * with a smaller one.  However, it can also fail to allocate the new pcb,
     * in which case it returns null and doesn't deallocate the original.
     * Because of this, we have to be careful with pcb ownership and the
     * callback argument here.
     */
    auto orig_pcb = m_pcb.release();
    /*
     * cache idx and transfer to "listen_pcb" since netif_idx is assigned the
     * DEFAULT value when listen_pcb is created
     */
    auto netif_idx = orig_pcb->netif_idx;

    ::tcp_arg(orig_pcb, nullptr);
    err_t error = ERR_OK;
    auto listen_pcb =
        tcp_listen_with_backlog_and_err(orig_pcb, backlog, &error);

    if (!listen_pcb) {
        m_pcb.reset(orig_pcb);
        ::tcp_arg(orig_pcb, this);
        return {tl::make_unexpected(err_to_errno(error)), std::nullopt};
    }

    listen_pcb->netif_idx = netif_idx;

    m_pcb.reset(listen_pcb);
    ::tcp_arg(listen_pcb, this);

    return {api::reply_success(), tcp_listening()};
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_listen& listen, const tcp_listening&)
{
    auto backlog = listen.backlog;
    if (backlog > tcp_backlog_max) {
        // Clip to max limit (same as Linux does)
        OP_LOG(OP_LOG_INFO,
               "TCP socket listen backlog %d is too large.  "
               "Using maximum supported value %d.",
               backlog,
               tcp_backlog_max);
        backlog = tcp_backlog_max;
    }

    if (m_pcb->state == LISTEN) {
        // Allow changing backlog when already in listen state
        auto lpcb = reinterpret_cast<struct tcp_pcb_listen*>(m_pcb.get());
        tcp_backlog_set(lpcb, backlog);
        return {api::reply_success(), tcp_listening()};
    }
    return {tl::make_unexpected(EINVAL), std::nullopt};
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_listen&,
                                                    const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

static tl::expected<void, int>
do_tcp_connect(tcp_pcb* pcb, const api::request_connect& connect)
{
    sockaddr_storage sstorage;

    auto copy_result =
        copy_in(sstorage, connect.id.pid, connect.name, connect.namelen);
    if (!copy_result) { return (tl::make_unexpected(copy_result.error())); }

    auto ip_addr = get_address(sstorage, IP_IS_V6_VAL(pcb->local_ip));
    auto ip_port = get_port(sstorage);

    if (!ip_addr | !ip_port) { return (tl::make_unexpected(EINVAL)); }

    auto error = tcp_connect(pcb, &*ip_addr, *ip_port, nullptr);
    if (error != ERR_OK) { return (tl::make_unexpected(err_to_errno(error))); }

    return {};
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_connect& connect, const tcp_init&)
{
    auto result = do_tcp_connect(m_pcb.get(), connect);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_working(), tcp_connecting()};
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_connect& connect, const tcp_bound&)
{
    auto result = do_tcp_connect(m_pcb.get(), connect);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_working(), tcp_connecting()};
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_connect&,
                                                    const tcp_connecting&)
{
    return {tl::make_unexpected(EALREADY), std::nullopt};
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_connect&,
                                                    const tcp_connected&)
{
    return {tl::make_unexpected(EISCONN), std::nullopt};
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_connect&,
                                                    const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_shutdown& shutdown,
                       const tcp_connected&)
{
    int shut_rx = 0, shut_tx = 0;
    switch (shutdown.how) {
    case SHUT_RD:
        shut_rx = 1;
        break;
    case SHUT_WR:
        shut_tx = 1;
        break;
    case SHUT_RDWR:
        shut_rx = 1;
        shut_tx = 1;
        break;
    default:
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }

    auto error = tcp_shutdown(m_pcb.get(), shut_rx, shut_tx);
    if (error != ERR_OK) {
        return {tl::make_unexpected(err_to_errno(error)), std::nullopt};
    }

    return {api::reply_success(), std::nullopt};
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_shutdown&, const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

template <typename NameRequest>
static tl::expected<socklen_t, int> do_tcp_get_name(const ip_addr_t& ip_addr,
                                                    const in_port_t& ip_port,
                                                    const NameRequest& request)
{
    struct sockaddr_storage sstorage;
    socklen_t slength = 0;

    switch (IP_GET_TYPE(&ip_addr)) {
    case IPADDR_TYPE_V4: {
        auto* sa4 = reinterpret_cast<sockaddr_in*>(&sstorage);
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons(ip_port);
        sa4->sin_addr.s_addr = ip_2_ip4(&ip_addr)->addr;

        slength = sizeof(sockaddr_in);
        break;
    }
    case IPADDR_TYPE_V6:
    case IPADDR_TYPE_ANY: {
        auto* sa6 = reinterpret_cast<sockaddr_in6*>(&sstorage);
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons(ip_port);
        std::memcpy(
            sa6->sin6_addr.s6_addr, ip_2_ip6(&ip_addr)->addr, sizeof(in6_addr));

        slength = sizeof(sockaddr_in6);
        break;
    }
    default:
        throw std::logic_error("Fix me!");
    }

    /* Copy the data out */
    auto result = copy_out(request.id.pid,
                           request.name,
                           sstorage,
                           std::min(request.namelen, slength));
    if (!result) { return (tl::make_unexpected(result.error())); }

    return (slength);
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_getpeername& getpeername,
                       const tcp_connected&)
{
    auto result =
        do_tcp_get_name(m_pcb->remote_ip, m_pcb->remote_port, getpeername);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_socklen{*result}, std::nullopt};
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_getpeername&, const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

tl::expected<socklen_t, int>
tcp_socket::do_tcp_getsockname(const tcp_pcb* pcb,
                               const api::request_getsockname& getsockname)
{
    return (do_tcp_get_name(pcb->local_ip, pcb->local_port, getsockname));
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_getsockname&, const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

static tl::expected<socklen_t, int>
do_tcp_getsockopt(const tcp_pcb* pcb, const api::request_getsockopt& getsockopt)
{
    assert(getsockopt.level == IPPROTO_TCP);

    socklen_t slength = 0;
    switch (getsockopt.optname) {
    case LINUX_TCP_NODELAY: {
        int opt = !!tcp_nagle_disabled(pcb);
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, opt);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(opt);
        break;
    }
    case LINUX_TCP_MAXSEG: {
        int mss = tcp_mss(pcb);
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, mss);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(mss);
        break;
    }
    case LINUX_TCP_INFO: {
        auto info = tcp_info{};
        get_tcp_info(pcb, info);
        auto result = copy_out(
            getsockopt.id.pid,
            getsockopt.optval,
            reinterpret_cast<void*>(&info),
            std::min(sizeof(info), static_cast<size_t>(getsockopt.optlen)));
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(info);
        break;
    }
    case LINUX_TCP_CONGESTION: {
        std::string_view cc = "reno";
        auto result = copy_out(
            getsockopt.id.pid,
            getsockopt.optval,
            cc.data(),
            std::min(cc.length(), static_cast<size_t>(getsockopt.optlen)));
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(cc.length());
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return (slength);
}

tl::expected<socklen_t, int>
tcp_socket::do_getsockopt(const tcp_pcb* pcb,
                          const api::request_getsockopt& getsockopt,
                          const tcp_socket_state& state)
{
    switch (getsockopt.level) {
    case SOL_SOCKET:
        switch (getsockopt.optname) {
        case SO_ERROR: {
            int error = 0;
            if (std::holds_alternative<tcp_error>(state)) {
                error = std::get<tcp_error>(state).value;
            }
            auto result = copy_out(getsockopt.id.pid, getsockopt.optval, error);
            if (!result) return (tl::make_unexpected(result.error()));
            return (sizeof(error));
        }
        case SO_TYPE: {
            int type = SOCK_STREAM;
            auto result = copy_out(getsockopt.id.pid, getsockopt.optval, type);
            if (!result) return (tl::make_unexpected(result.error()));
            return (sizeof(type));
        }
        default:
            return (do_sock_getsockopt(reinterpret_cast<const ip_pcb*>(pcb),
                                       getsockopt));
        }
    case IPPROTO_IP:
        return (
            do_ip_getsockopt(reinterpret_cast<const ip_pcb*>(pcb), getsockopt));
    case IPPROTO_IPV6:
        return (do_ip6_getsockopt(reinterpret_cast<const ip_pcb*>(pcb),
                                  getsockopt));
    case IPPROTO_TCP:
        return (do_tcp_getsockopt(pcb, getsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

static tl::expected<void, int>
do_tcp_setsockopt(tcp_pcb* pcb, const api::request_setsockopt& setsockopt)
{
    assert(setsockopt.level == IPPROTO_TCP);

    switch (setsockopt.optname) {
    case LINUX_TCP_NODELAY: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt)
            tcp_nagle_disable(pcb);
        else
            tcp_nagle_enable(pcb);
        break;
    }
    case LINUX_TCP_MAXSEG: {
        auto mss = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!mss) return (tl::make_unexpected(mss.error()));
        if (*mss < 536) return (tl::make_unexpected(EINVAL));
        tcp_mss(pcb) = *mss;
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return {};
}

tl::expected<void, int>
tcp_socket::do_setsockopt(tcp_pcb* pcb,
                          const api::request_setsockopt& setsockopt)
{
    switch (setsockopt.level) {
    case SOL_SOCKET:
        return (do_sock_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case IPPROTO_IP:
        return (do_ip_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case IPPROTO_IPV6:
        return (do_ip6_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case IPPROTO_TCP:
        return (do_tcp_setsockopt(pcb, setsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

tcp_socket::on_request_reply
tcp_socket::on_request(const api::request_setsockopt&, const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

} // namespace openperf::socket::server
