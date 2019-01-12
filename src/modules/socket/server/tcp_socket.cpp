#include <cstring>
#include <numeric>

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

static size_t do_tcp_transmit(tcp_pcb* pcb, void* ptr, size_t length)
{
    auto to_write = std::min(static_cast<uint16_t>(length), tcp_sndbuf(pcb));
    auto error = tcp_write(pcb, ptr, to_write, TCP_WRITE_FLAG_COPY);
    return (error == ERR_OK ? to_write : 0);
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

tcp_socket::tcp_socket(icp::memory::allocator::pool* pool, pid_t pid, tcp_pcb* pcb)
    : m_channel(new (pool->acquire()) stream_channel(), stream_channel_deleter(pool))
    , m_pcb(pcb)
    , m_pid(pid)
{
    ::tcp_arg(pcb, this);
    ::tcp_poll(m_pcb.get(), nullptr, 2U);
    state(tcp_connected());
}

tcp_socket::tcp_socket(icp::memory::allocator::pool& pool, pid_t pid)
    : m_channel(new (pool.acquire()) stream_channel(), stream_channel_deleter(&pool))
    , m_pcb(tcp_new())
    , m_pid(pid)
{
    ::tcp_arg(m_pcb.get(), this);
    ::tcp_poll(m_pcb.get(), nullptr, 2U);
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
    }
    return (*this);
}

tcp_socket::tcp_socket(tcp_socket&& other) noexcept
    : m_channel(std::move(other.m_channel))
    , m_pcb(std::move(other.m_pcb))
    , m_pid(other.m_pid)
    , m_acceptq(std::move(other.m_acceptq))
{
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

    m_acceptq.emplace(tcp_socket(channel_pool(), m_pid, newpcb));
    m_channel->notify();
    tcp_backlog_delayed(m_pcb.get());

    return (ERR_OK);
}

int tcp_socket::do_lwip_sent(uint16_t size __attribute__((unused)))
{
    // noop
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
        return (ERR_OK);
    }

    auto sendvec = iovec{ .iov_base = p->payload,
                          .iov_len = p->len };
    auto success = m_channel->send(m_pid, &sendvec, 1);
    pbuf_free(p);
    return (success ? ERR_OK : ERR_MEM);
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
    if (length) {
        m_channel->recv_skip(do_tcp_transmit(m_pcb.get(), ptr, length));
    }

    return (ERR_OK);
}

int tcp_socket::do_lwip_error(int err)
{
    state(tcp_error{ .error = err_to_errno(err) });
    /*
     * lwIP will "conveniently" free the pbuf for us before this callback is
     * triggered, so make sure we don't.
     */
    m_pcb.release();
    return (ERR_OK);
}

tl::expected<generic_socket, int> tcp_socket::handle_accept()
{
    if (!std::holds_alternative<tcp_listening>(state())) {
        return (tl::make_unexpected(EINVAL));
    }

    if (m_acceptq.empty()) {
        return (tl::make_unexpected(EAGAIN));
    }

    auto& socket = m_acceptq.front();
    m_acceptq.pop();
    tcp_backlog_accepted(m_pcb.get());
    return (generic_socket(std::move(socket)));
}

void tcp_socket::handle_transmit()
{
    m_channel->ack();
    auto [ptr, length] = m_channel->recv_peek();
    if (!length) return;

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

tl::expected<socklen_t, int> tcp_socket::do_tcp_getsockname(tcp_pcb* pcb,
                                                            const api::request_getsockname& getsockname)
{
    return (do_tcp_get_name(pcb->local_ip, pcb->local_port, getsockname));
}

tl::expected<socklen_t, int> tcp_socket::do_tcp_getsockopt(tcp_pcb* pcb,
                                                           const api::request_getsockopt& getsockopt)
{
    socklen_t slength = 0;
    switch (getsockopt.optname) {
    case SO_BROADCAST: {
        int opt = !!ip_get_option(pcb, SOF_BROADCAST);
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, opt);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(opt);
        break;
    }
    case SO_REUSEADDR: {
        int opt = !!ip_get_option(pcb, SOF_REUSEADDR);
        auto result = copy_out(getsockopt.id.pid, getsockopt.optval, opt);
        if (!result) return (tl::make_unexpected(result.error()));
        slength = sizeof(opt);
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return (slength);
}

tl::expected<void, int> tcp_socket::do_tcp_setsockopt(tcp_pcb* pcb,
                                                      const api::request_setsockopt& setsockopt)
{
    switch (setsockopt.optname) {
    case SO_BROADCAST: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt) ip_set_option(pcb, SOF_BROADCAST);
        else      ip_reset_option(pcb, SOF_BROADCAST);
        break;
    }
    case SO_REUSEADDR: {
        auto opt = copy_in(setsockopt.id.pid,
                           reinterpret_cast<const int*>(setsockopt.optval));
        if (!opt) return (tl::make_unexpected(opt.error()));
        if (*opt) ip_set_option(pcb, SOF_REUSEADDR);
        else      ip_reset_option(pcb, SOF_REUSEADDR);
        break;
    }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }

    return {};
}

}
}
}
