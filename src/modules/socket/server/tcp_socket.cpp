#include <cstring>
#include <numeric>

#include "socket/server/compat/linux/tcp.h"
#include "socket/server/lwip_utils.h"
#include "socket/server/tcp_socket.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

namespace icp {
namespace socket {
namespace server {

const char * to_string(const tcp_socket_state& state)
{
    return (std::visit(overloaded_visitor(
                           [](const tcp_init&) -> const char* {
                               return ("init");
                           },
                           [](const tcp_bound&) -> const char* {
                               return ("bound");
                           },
                           [](const tcp_accept&) -> const char* {
                               return ("accept");
                           },
                           [](const tcp_listening&) -> const char* {
                               return ("listening");
                           },
                           [](const tcp_connecting&) -> const char* {
                               return ("connecting");
                           },
                           [](const tcp_connected&) -> const char* {
                               return ("connected");
                           },
                           [](const tcp_closing&) -> const char* {
                               return ("closing");
                           },
                           [](const tcp_closed&) -> const char* {
                               return ("closed");
                           },
                           [](const tcp_error&) -> const char* {
                               return ("error");
                           }),
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

static size_t do_tcp_transmit(tcp_pcb* pcb, void* ptr, size_t length)
{
    if (length == 0
        || tcp_sndbuf(pcb) <= TCP_SNDLOWAT
        || tcp_sndqueuelen(pcb) >= TCP_SNDQUEUELOWAT) {
        /* skip transmitting at this time... */
        return (0);
    }

    auto to_write = std::min(length, static_cast<size_t>(tcp_sndbuf(pcb)));
    if (tcp_write(pcb, ptr, to_write, TCP_WRITE_FLAG_COPY) != ERR_OK) {
        return (0);
    }

    if (to_write == length) {
        /* If we wrote everything we were asked to write, then trigger a transmission */
        tcp_output(pcb);
    }

    return (to_write);
}

void tcp_socket::tcp_pcb_deleter::operator()(tcp_pcb *pcb)
{
     /* quick and dirty */
    if (tcp_close(pcb) != ERR_OK) {
        tcp_abort(pcb);
    }
}

icp::memory::allocator::pool* tcp_socket::channel_pool()
{
    auto deleter = reinterpret_cast<stream_channel_deleter*>(&m_channel.get_deleter());
    return (deleter->pool_);
}

tcp_socket::tcp_socket(icp::memory::allocator::pool* pool, pid_t pid, int flags, tcp_pcb* pcb)
    : m_channel(new (pool->acquire()) stream_channel(flags), stream_channel_deleter(pool))
    , m_pcb(pcb)
    , m_pid(pid)
{
    ::tcp_arg(pcb, this);
    ::tcp_poll(m_pcb.get(), nullptr, 2U);
    state(tcp_connected());
}

tcp_socket::tcp_socket(icp::memory::allocator::pool& pool, pid_t pid, int flags)
    : m_channel(new (pool.acquire()) stream_channel(flags), stream_channel_deleter(&pool))
    , m_pcb(tcp_new())
    , m_pid(pid)
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
    ::tcp_arg(m_pcb.get(), nullptr);
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
        m_pid = other.m_pid;
        m_acceptq = std::move(other.m_acceptq);
        ::tcp_arg(m_pcb.get(), this);
        state(other.state());
    }
    return (*this);
}

tcp_socket::tcp_socket(tcp_socket&& other) noexcept
    : m_channel(std::move(other.m_channel))
    , m_pcb(std::move(other.m_pcb))
    , m_pid(other.m_pid)
    , m_acceptq(std::move(other.m_acceptq))
{
    state(other.state());
    ::tcp_arg(m_pcb.get(), this);
}

channel_variant tcp_socket::channel() const
{
    return (m_channel.get());
}

int tcp_socket::do_lwip_accept(tcp_pcb *newpcb, int err)
{
    if (err != ERR_OK || newpcb == nullptr) {
        return (ERR_VAL);
    }

    m_acceptq.emplace(newpcb);
    tcp_backlog_delayed(newpcb);
    m_channel->notify();

    return (ERR_OK);
}

int tcp_socket::do_lwip_sent(uint16_t size)
{
    /*
     * Size bytes just cleared the TCP send buffer.  Let's see
     * if we have up to size bytes more to send.
     */
    auto [ptr, length] = m_channel->recv_peek();
    auto to_send = std::min(length, static_cast<size_t>(size));
    m_channel->recv_skip(do_tcp_transmit(m_pcb.get(), ptr, to_send));
    return (ERR_OK);
}

int tcp_socket::do_lwip_recv(pbuf *p, int err)
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

    std::array<iovec, 1024> iovecs;
    size_t iovcnt = 0;

    auto p_current = p;
    while (p_current != nullptr) {
        iovecs[iovcnt++] = iovec{ .iov_base = p_current->payload,
                                  .iov_len = p_current->len };
        p_current = p_current->next;
    }
    assert(iovcnt == pbuf_clen(p));

    if (!m_channel->send(m_pid, iovecs.data(), iovcnt)) {
        return (ERR_MEM);
    }

    pbuf_free(p);
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
    return (ERR_OK);
}

int tcp_socket::do_lwip_poll()
{
    /* need to check idle time and close if necessary */

    /* need to check if we can send more data */
    auto [ptr, length] = m_channel->recv_peek();
    m_channel->recv_skip(do_tcp_transmit(m_pcb.get(), ptr, length));

    return (ERR_OK);
}

int tcp_socket::do_lwip_error(int err)
{
    m_channel->error(err_to_errno(err));
    state(tcp_error{ .value = err_to_errno(err) });
    /*
     * lwIP will "conveniently" free the pbuf for us before this callback is
     * triggered, so make sure we don't.
     */
    m_pcb.release();
    return (ERR_OK);
}

tl::expected<generic_socket, int> tcp_socket::handle_accept(int flags)
{
    if (!std::holds_alternative<tcp_listening>(state())) {
        return (tl::make_unexpected(EINVAL));
    }

    if (m_acceptq.empty()) {
        return (tl::make_unexpected(EAGAIN));
    }

    auto pcb = m_acceptq.front();
    m_acceptq.pop();
    tcp_backlog_accepted(pcb);
    return (generic_socket(tcp_socket(channel_pool(), m_pid, flags, pcb)));
}

void tcp_socket::handle_transmit()
{
    if (!m_pcb) return;  /* can be closed on error */

    m_channel->ack();

    if (auto to_ack = m_channel->send_ack(); to_ack != 0) {
        tcp_recved(m_pcb.get(), to_ack);
    }

    auto [ptr, length] = m_channel->recv_peek();
    m_channel->recv_skip(do_tcp_transmit(m_pcb.get(), ptr, length));
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_bind& bind,
                                                    const tcp_init&)
{
    sockaddr_storage sstorage;

    auto copy_result = copy_in(sstorage, bind.id.pid, bind.name, bind.namelen);
    if (!copy_result) {
        return {tl::make_unexpected(copy_result.error()), std::nullopt};
    }

    auto ip_addr = get_address(sstorage);
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

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_listen& listen,
                                                    const tcp_bound&)
{
    /*
     * The lwIP listen function deallocates the original pcb and replaces it with
     * a smaller one.  However, it can also fail to allocate the new pcb, in which case
     * it returns null and doesn't deallocate the original.  Because of this, we have
     * to be careful with pcb ownership here.
     */
    auto orig_pcb = m_pcb.release();
    auto listen_pcb = tcp_listen_with_backlog(orig_pcb, listen.backlog);

    if (!listen_pcb) {
        m_pcb.reset(orig_pcb);
        return {tl::make_unexpected(ENOMEM), std::nullopt};
    }

    m_pcb.reset(listen_pcb);

    return {api::reply_success(), tcp_listening()};
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_listen&,
                                                    const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

static tl::expected<void, int> do_tcp_connect(tcp_pcb *pcb, const api::request_connect& connect)
{
    sockaddr_storage sstorage;

    auto copy_result = copy_in(sstorage, connect.id.pid, connect.name, connect.namelen);
    if (!copy_result) {
        return (tl::make_unexpected(copy_result.error()));
    }

    auto ip_addr = get_address(sstorage);
    auto ip_port = get_port(sstorage);

    if (!ip_addr | !ip_port) {
        return (tl::make_unexpected(EINVAL));
    }

    auto error = tcp_connect(pcb, &*ip_addr, *ip_port, nullptr);
    if (error != ERR_OK) {
        return (tl::make_unexpected(err_to_errno(error)));
    }

    return {};
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_connect& connect,
                                                    const tcp_init&)
{
    auto result = do_tcp_connect(m_pcb.get(), connect);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_success(), tcp_connecting()};
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_connect& connect,
                                                    const tcp_bound&)
{
    auto result = do_tcp_connect(m_pcb.get(), connect);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_success(), tcp_connecting()};
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

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_shutdown& shutdown,
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

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_shutdown&,
                                                    const tcp_error& error)
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
        struct sockaddr_in *sa4 = reinterpret_cast<sockaddr_in*>(&sstorage);
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons(ip_port);
        sa4->sin_addr.s_addr = ip_2_ip4(&ip_addr)->addr;

        slength = sizeof(sockaddr_in);
        break;
    }
    case IPADDR_TYPE_V6: {
        struct sockaddr_in6 *sa6 = reinterpret_cast<sockaddr_in6*>(&sstorage);
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons(ip_port);
        std::memcpy(sa6->sin6_addr.s6_addr,
                    ip_2_ip6(&ip_addr)->addr,
                    sizeof(in6_addr));

        slength = sizeof(sockaddr_in6);
        break;
    }
    default:
        throw std::logic_error("Fix me!");
    }

    /* Copy the data out */
    auto result = copy_out(request.id.pid, request.name,
                           sstorage, std::min(request.namelen, slength));
    if (!result) {
        return (tl::make_unexpected(result.error()));
    }

    return (slength);
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_getpeername& getpeername,
                                                    const tcp_connected&)
{
    auto result = do_tcp_get_name(m_pcb->remote_ip, m_pcb->remote_port, getpeername);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_socklen{*result}, std::nullopt};
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_getpeername&,
                                                    const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

tl::expected<socklen_t, int> tcp_socket::do_tcp_getsockname(const tcp_pcb* pcb,
                                                            const api::request_getsockname& getsockname)
{
    return (do_tcp_get_name(pcb->local_ip, pcb->local_port, getsockname));
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_getsockname&,
                                                    const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

static
tl::expected<socklen_t, int> do_tcp_getsockopt(const tcp_pcb* pcb,
                                               const api::request_getsockopt& getsockopt)
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
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval,
                               reinterpret_cast<void*>(&info),
                               std::min(sizeof(info), static_cast<size_t>(getsockopt.optlen)));
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(info);
        break;
    }
    case LINUX_TCP_CONGESTION: {
        std::string_view cc = "reno";
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, cc.data(),
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

tl::expected<socklen_t, int> tcp_socket::do_getsockopt(const tcp_pcb* pcb,
                                                       const api::request_getsockopt& getsockopt)
{
    switch (getsockopt.level) {
    case SOL_SOCKET:
        return (do_sock_getsockopt(reinterpret_cast<const ip_pcb*>(pcb), getsockopt));
    case IPPROTO_IP:
        return (do_ip_getsockopt(reinterpret_cast<const ip_pcb*>(pcb), getsockopt));
    case IPPROTO_TCP:
        return (do_tcp_getsockopt(pcb, getsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_getsockopt&,
                                                    const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

static
tl::expected<void, int> do_tcp_setsockopt(tcp_pcb* pcb,
                                          const api::request_setsockopt& setsockopt)
{
    assert(setsockopt.level == IPPROTO_TCP);

    switch (setsockopt.optname) {
    case LINUX_TCP_NODELAY: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt) tcp_nagle_disable(pcb);
        else      tcp_nagle_enable(pcb);
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

tl::expected<void, int> tcp_socket::do_setsockopt(tcp_pcb* pcb,
                                                  const api::request_setsockopt& setsockopt)
{
    switch (setsockopt.level) {
    case SOL_SOCKET:
        return (do_sock_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case IPPROTO_IP:
        return (do_ip_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case IPPROTO_TCP:
        return (do_tcp_setsockopt(pcb, setsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

tcp_socket::on_request_reply tcp_socket::on_request(const api::request_setsockopt&,
                                                    const tcp_error& error)
{
    return {tl::make_unexpected(error.value), std::nullopt};
}

}
}
}
