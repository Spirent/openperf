#include <cassert>
#include <cstring>
#include <numeric>
#include <optional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "socket/server/lwip_utils.h"
#include "socket/server/udp_socket.h"
#include "lwip/memp.h"
#include "lwip/udp.h"
#include "utils/overloaded_visitor.h"

namespace openperf {
namespace socket {
namespace server {

const char * to_string(const udp_socket_state& state)
{
    return (std::visit(utils::overloaded_visitor(
                           [](const udp_init&) -> const char* {
                               return ("init");
                           },
                           [](const udp_bound&) -> const char* {
                               return ("bound");
                           },
                           [](const udp_connected&) -> const char* {
                               return ("connected");
                           },
                           [](const udp_closed&) -> const char * {
                               return ("closed");
                           }),
                       state));
}

void udp_socket::udp_pcb_deleter::operator()(udp_pcb *pcb)
{
    udp_remove(pcb);
}

static void udp_receive(void* arg, udp_pcb* pcb, pbuf* p, const ip_addr_t* addr, in_port_t port)
{
    (void)pcb;
    assert(addr != nullptr);
    auto channel = reinterpret_cast<dgram_channel*>(arg);
    /*
     * Note: we switch the byte order of the port here because lwip provides the port
     * in host byte order but BSD socket clients expect it to be in network byte
     * order.
     */
    if (!channel->send(p, reinterpret_cast<const dgram_ip_addr*>(addr), htons(port))) {
        UDP_STATS_INC(udp.drop);
        pbuf_free(p);
    }
}

udp_socket::udp_socket(openperf::socket::server::allocator& allocator, int flags)
    : m_channel(new (allocator.allocate(sizeof(dgram_channel)))
                dgram_channel(flags, allocator))
    , m_pcb(udp_new())
{
    if (!m_pcb) {
        throw std::runtime_error("Out of UDP pcb's!");
    }

    udp_recv(m_pcb.get(), &udp_receive, m_channel.get());
}

channel_variant udp_socket::channel() const
{
    return (m_channel.get());
}

tl::expected<generic_socket, int> udp_socket::handle_accept(int)
{
    /* Can't accept on a UDP socket */
    return (tl::make_unexpected(EOPNOTSUPP));
}

void udp_socket::handle_io()
{
    m_channel->ack();
    while (m_channel->recv_available()) {
        auto [p, dest] = m_channel->recv();

        assert(p);

        if (dest) udp_sendto(m_pcb.get(), p,
                             reinterpret_cast<const ip_addr_t*>(&dest->addr()),
                             dest->port());
        else      udp_send(m_pcb.get(), p);

        pbuf_free(p);
    }
}

static tl::expected<void, int> do_udp_bind(udp_pcb* pcb, const api::request_bind& bind)
{
    sockaddr_storage sstorage;

    auto copy_result = copy_in(sstorage, bind.id.pid, bind.name, bind.namelen);
    if (!copy_result) {
        return (tl::make_unexpected(copy_result.error()));
    }

    auto ip_addr = get_address(sstorage);
    auto ip_port = get_port(sstorage);

    if (!ip_addr || !ip_port) {
        return (tl::make_unexpected(EINVAL));
    }

    auto error = udp_bind(pcb, &*ip_addr, *ip_port);
    if (error != ERR_OK) {
        return (tl::make_unexpected(err_to_errno(error)));
    }

    return {};
}

/* provide udp_disconnect a return type so that it's less cumbersome to use */
static err_t do_udp_disconnect(udp_pcb* pcb)
{
    udp_disconnect(pcb);
    return (ERR_OK);
}

static tl::expected<bool, int> do_udp_connect(udp_pcb* pcb,
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
    auto ip_port = get_port(sstorage);

    if (!ip_addr || !ip_port) {
        return (tl::make_unexpected(EINVAL));
    }

    /* connection-less sockets allow both connects and disconnects */
    auto error = (IP_IS_ANY_TYPE_VAL(*ip_addr)
                  ? do_udp_disconnect(pcb)
                  : udp_connect(pcb, &*ip_addr, *ip_port));
    if (error != ERR_OK) {
        return (tl::make_unexpected(err_to_errno(error)));
    }

    return (!IP_IS_ANY_TYPE_VAL(*ip_addr));
}

template <typename NameRequest>
static tl::expected<socklen_t, int> do_udp_get_name(const ip_addr_t& ip_addr,
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

tl::expected<socklen_t, int> udp_socket::do_getsockopt(const udp_pcb* pcb,
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

tl::expected<void, int> udp_socket::do_setsockopt(udp_pcb* pcb,
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

udp_socket::on_request_reply udp_socket::on_request(const api::request_bind& bind,
                                                    const udp_init&)
{
    auto result = do_udp_bind(m_pcb.get(), bind);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_success(), udp_bound()};
}

udp_socket::on_request_reply udp_socket::on_request(const api::request_connect& connect,
                                                    const udp_init&)
{
    auto result = do_udp_connect(m_pcb.get(), connect, m_channel.get());
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    if (!*result) return {api::reply_success(), std::nullopt};  /* still not connected */
    return {api::reply_success(), udp_connected()};
}

udp_socket::on_request_reply udp_socket::on_request(const api::request_connect& connect,
                                                    const udp_bound&)
{
    auto result = do_udp_connect(m_pcb.get(), connect, m_channel.get());
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    if (!*result) return {api::reply_success(), std::nullopt};  /* still not connected */
    return {api::reply_success(), udp_connected()};
}

udp_socket::on_request_reply udp_socket::on_request(const api::request_connect& connect,
                                                    const udp_connected&)
{
    auto result = do_udp_connect(m_pcb.get(), connect, m_channel.get());
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    if (!*result) return {api::reply_success(), udp_bound()}; /* disconnected */
    return {api::reply_success(), udp_connected()};
}

udp_socket::on_request_reply udp_socket::on_request(const api::request_getpeername& getpeername,
                                                    const udp_connected&)
{
    auto result = do_udp_get_name(m_pcb->remote_ip, m_pcb->remote_port,
                                  getpeername);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_socklen{*result}, std::nullopt};
}

udp_socket::on_request_reply udp_socket::on_request(const api::request_getsockname& getsockname,
                                                    const udp_bound&)
{
    auto result = do_udp_get_name(m_pcb->local_ip, m_pcb->local_port,
                                  getsockname);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_socklen{*result}, std::nullopt};
}

udp_socket::on_request_reply udp_socket::on_request(const api::request_getsockname& getsockname,
                                                    const udp_connected&)
{
    auto result = do_udp_get_name(m_pcb->local_ip, m_pcb->local_port,
                                  getsockname);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_socklen{*result}, std::nullopt};
}

}
}
}
