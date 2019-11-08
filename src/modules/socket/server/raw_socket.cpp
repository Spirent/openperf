#include <cassert>
#include <cstring>
#include <numeric>
#include <optional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "socket/server/lwip_utils.h"
#include "socket/server/raw_socket.h"
#include "lwip/memp.h"
#include "lwip/raw.h"


namespace icp {
namespace socket {
namespace server {

const char * to_string(const raw_socket_state& state)
{
    return (std::visit(overloaded_visitor(
                           [](const raw_init&) -> const char* {
                               return ("init");
                           },
                           [](const raw_bound&) -> const char* {
                               return ("bound");
                           },
                           [](const raw_connected&) -> const char* {
                               return ("connected");
                           },
                           [](const raw_closed&) -> const char* {
                               return ("closed");
                           }),
                       state));
}

void raw_socket::raw_pcb_deleter::operator()(raw_pcb *pcb)
{
    raw_remove(pcb);
}

static uint8_t raw_receive(void* arg, raw_pcb* pcb, pbuf* p, const ip_addr_t* addr)
{
    (void)pcb;
    assert(addr != nullptr);
    auto sock = reinterpret_cast<raw_socket*>(arg);

    auto channel = std::get<dgram_channel*>(sock->channel());
    if (!channel->send(p, reinterpret_cast<const dgram_ip_addr*>(addr), 0)) {
        return (0);
    }
    return (1);
}

raw_socket::raw_socket(icp::socket::server::allocator& allocator, int flags, int protocol, raw_recv_fn recv_callback)
    : m_channel(new (allocator.allocate(sizeof(dgram_channel)))
                dgram_channel(flags, allocator))
    , m_pcb(raw_new(protocol))
    , m_recv_callback(recv_callback)
{
    if (!m_pcb) {
        throw std::runtime_error("Out of RAW pcb's!");
    }
    if (m_recv_callback == nullptr) m_recv_callback = &raw_receive;
    raw_recv(m_pcb.get(), m_recv_callback, this);
}

raw_socket& raw_socket::operator=(raw_socket&& other) noexcept
{
    if (this != &other) {
        m_channel = std::move(other.m_channel);
        m_pcb = std::move(other.m_pcb);
        m_recv_callback = other.m_recv_callback;
        raw_recv(m_pcb.get(), m_recv_callback, this);
    }
    return (*this);
}

raw_socket::raw_socket(raw_socket&& other) noexcept
    : m_channel(std::move(other.m_channel))
    , m_pcb(std::move(other.m_pcb))
    , m_recv_callback(other.m_recv_callback)
{
    raw_recv(m_pcb.get(), m_recv_callback, this);
}

channel_variant raw_socket::channel() const
{
    return (m_channel.get());
}

tl::expected<generic_socket, int> raw_socket::handle_accept(int)
{
    /* Can't accept on a RAW socket */
    return (tl::make_unexpected(EOPNOTSUPP));
}

void raw_socket::handle_io()
{
    m_channel->ack();
    while (m_channel->recv_available()) {
        auto [p, dest] = m_channel->recv();

        assert(p);

        if (dest) raw_sendto(m_pcb.get(), p, reinterpret_cast<const ip_addr_t*>(&dest->addr()));
        else      raw_send(m_pcb.get(), p);

        pbuf_free(p);
    }
}

static tl::expected<void, int> do_raw_bind(raw_pcb* pcb, const api::request_bind& bind)
{
    sockaddr_storage sstorage;

    auto copy_result = copy_in(sstorage, bind.id.pid, bind.name, bind.namelen);
    if (!copy_result) {
        return (tl::make_unexpected(copy_result.error()));
    }

    auto ip_addr = get_address(sstorage);

    if (!ip_addr) {
        return (tl::make_unexpected(EINVAL));
    }

    auto error = raw_bind(pcb, &*ip_addr);
    if (error != ERR_OK) {
        return (tl::make_unexpected(err_to_errno(error)));
    }

    return {};
}

/* provide raw_disconnect a return type so that it's less cumbersome to use */
static err_t do_raw_disconnect(raw_pcb* pcb)
{
    raw_disconnect(pcb);
    return (ERR_OK);
}

static tl::expected<bool, int> do_raw_connect(raw_pcb* pcb,
                                              const api::request_connect& connect,
                                              dgram_channel* channel)
{
    sockaddr_storage sstorage;

    /* always unblock the channel */
    channel->unblock();

    /* unblock clears any pending notifications; check if we should (re)notify */
    if (!channel->send_empty()) {
        channel->notify();
    }

    auto copy_result = copy_in(sstorage, connect.id.pid, connect.name, connect.namelen);
    if (!copy_result) {
        return (tl::make_unexpected(copy_result.error()));
    }

    auto ip_addr = get_address(sstorage);

    if (!ip_addr) {
        return (tl::make_unexpected(EINVAL));
    }

    /* connection-less sockets allow both connects and disconnects */
    auto error = (IP_IS_ANY_TYPE_VAL(*ip_addr)
                  ? do_raw_disconnect(pcb)
                  : raw_connect(pcb, &*ip_addr));
    if (error != ERR_OK) {
        return (tl::make_unexpected(err_to_errno(error)));
    }

    return (!IP_IS_ANY_TYPE_VAL(*ip_addr));
}

template <typename NameRequest>
static tl::expected<socklen_t, int> do_raw_get_name(const ip_addr_t& ip_addr,
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

tl::expected<socklen_t, int> raw_socket::do_getsockopt(const raw_pcb* pcb,
                                                       const api::request_getsockopt& getsockopt)
{
    switch (getsockopt.level) {
    case SOL_SOCKET:
        return (do_sock_getsockopt(reinterpret_cast<const ip_pcb*>(pcb), getsockopt));
    case IPPROTO_IP:
        return (do_ip_getsockopt(reinterpret_cast<const ip_pcb*>(pcb), getsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

tl::expected<void, int> raw_socket::do_setsockopt(raw_pcb* pcb,
                                                  const api::request_setsockopt& setsockopt)
{
    switch (setsockopt.level) {
    case SOL_SOCKET:
        return (do_sock_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case IPPROTO_IP:
        return (do_ip_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

raw_socket::on_request_reply raw_socket::on_request(const api::request_bind& bind,
                                                    const raw_init&)
{
    auto result = do_raw_bind(m_pcb.get(), bind);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_success(), raw_bound()};
}

raw_socket::on_request_reply raw_socket::on_request(const api::request_connect& connect,
                                                    const raw_init&)
{
    auto result = do_raw_connect(m_pcb.get(), connect, m_channel.get());
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    if (!*result) return {api::reply_success(), std::nullopt};  /* still not connected */
    return {api::reply_success(), raw_connected()};
}

raw_socket::on_request_reply raw_socket::on_request(const api::request_connect& connect,
                                                    const raw_bound&)
{
    auto result = do_raw_connect(m_pcb.get(), connect, m_channel.get());
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    if (!*result) return {api::reply_success(), std::nullopt};  /* still not connected */
    return {api::reply_success(), raw_connected()};
}

raw_socket::on_request_reply raw_socket::on_request(const api::request_connect& connect,
                                                    const raw_connected&)
{
    auto result = do_raw_connect(m_pcb.get(), connect, m_channel.get());
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    if (!*result) return {api::reply_success(), raw_bound()}; /* disconnected */
    return {api::reply_success(), raw_connected()};
}

raw_socket::on_request_reply raw_socket::on_request(const api::request_getpeername& getpeername,
                                                    const raw_connected&)
{
    auto result = do_raw_get_name(m_pcb->remote_ip, 0, getpeername);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_socklen{*result}, std::nullopt};
}

raw_socket::on_request_reply raw_socket::on_request(const api::request_getsockname& getsockname,
                                                    const raw_bound&)
{
    auto result = do_raw_get_name(m_pcb->local_ip, 0, getsockname);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_socklen{*result}, std::nullopt};
}

raw_socket::on_request_reply raw_socket::on_request(const api::request_getsockname& getsockname,
                                                    const raw_connected&)
{
    auto result = do_raw_get_name(m_pcb->local_ip, 0, getsockname);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_socklen{*result}, std::nullopt};
}

}
}
}
