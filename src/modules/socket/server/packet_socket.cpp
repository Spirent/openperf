#include <linux/if_packet.h>

#include "packet/stack/lwip/packet.h"
#include "socket/server/lwip_utils.hpp"
#include "socket/server/packet_socket.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::socket::server {

/*
 * We use dgram_sockaddr_ll in most places to minimize our linux depdendency.
 * However, this file is one of the few places where both are actually
 * available, so perform some sanity checks to make sure they're still the same.
 */
static_assert(sizeof(sockaddr_ll) == sizeof(dgram_sockaddr_ll));

const char* to_string(const packet_socket_state& state)
{
    return (std::visit(
        utils::overloaded_visitor(
            [](const packet_init&) -> const char* { return ("init"); },
            [](const packet_bound&) -> const char* { return ("bound"); },
            [](const packet_closed&) -> const char* { return ("closed"); }),
        state));
}

static err_t
packet_receive(void* arg, struct pbuf* p, const struct sockaddr_ll* sll)
{
    assert(arg);
    assert(p);
    assert(sll);

    auto* channel = reinterpret_cast<dgram_channel*>(arg);

    if (!channel->send(p, reinterpret_cast<const dgram_sockaddr_ll*>(sll))) {
        pbuf_free(p);
        return (ERR_BUF);
    }

    return (ERR_OK);
}

packet_type_t to_packet_type(int socket_type)
{
    switch (socket_type & 0xFF) {
    case SOCK_DGRAM:
        return (PACKET_TYPE_DGRAM);
    case SOCK_RAW:
        return (PACKET_TYPE_RAW);
    default:
        return (PACKET_TYPE_NONE);
    }
}

packet_socket::packet_socket(openperf::socket::server::allocator& allocator,
                             int type_flags,
                             int protocol)
    : m_channel(new (allocator.allocate(sizeof(dgram_channel)))
                    dgram_channel(type_flags, allocator))
    , m_pcb(packet_new(to_packet_type(type_flags), protocol))
{
    if (!m_pcb) { throw std::runtime_error("No packet pcb!"); }

    packet_recv(m_pcb, &packet_receive, m_channel.get());
}

packet_socket::~packet_socket()
{
    if (m_pcb) { packet_remove(m_pcb); }
}

packet_socket::packet_socket(packet_socket&& other) noexcept
    : m_channel(std::move(other.m_channel))
    , m_pcb(other.m_pcb)
{
    other.m_pcb = nullptr;
}

packet_socket& packet_socket::operator=(packet_socket&& other) noexcept
{
    if (this != &other) {
        m_channel = std::move(other.m_channel);
        m_pcb = other.m_pcb;
        other.m_pcb = nullptr;
    }
    return (*this);
}

channel_variant packet_socket::channel() const { return (m_channel.get()); }

tl::expected<generic_socket, int> packet_socket::handle_accept(int)
{
    /* Can't accept on a packet socket */
    return (tl::make_unexpected(EOPNOTSUPP));
}

void packet_socket::handle_io()
{
    m_channel->ack();
    while (m_channel->recv_available()) {
        auto [p, dest] = m_channel->recv_ll();

        assert(p);

        if (dest) {
            packet_sendto(m_pcb,
                          p,
                          reinterpret_cast<const sockaddr_ll*>(
                              std::addressof(dest.value())));
        } else {
            packet_send(m_pcb, p);
        }

        pbuf_free(p);
    }
}

static tl::expected<void, int> do_packet_bind(packet_pcb* pcb,
                                              const api::request_bind& bind)
{

    if (bind.namelen != sizeof(sockaddr_ll)) {
        return (tl::make_unexpected(EINVAL));
    }

    sockaddr_storage sstorage;
    auto copy_result = copy_in(sstorage, bind.id.pid, bind.name, bind.namelen);
    if (!copy_result) { return (tl::make_unexpected(copy_result.error())); }

    auto* sll = reinterpret_cast<sockaddr_ll*>(std::addressof(sstorage));
    if (sll->sll_family != AF_PACKET) { return (tl::make_unexpected(EINVAL)); }

    auto error = packet_bind(pcb, sll->sll_protocol, sll->sll_ifindex);
    if (error != ERR_OK) { return (tl::make_unexpected(err_to_errno(error))); }

    return {};
}

tl::expected<socklen_t, int>
packet_socket::do_getsockopt(const packet_pcb* pcb,
                             const api::request_getsockopt& getsockopt)
{
    switch (getsockopt.level) {
    case SOL_SOCKET:
        if (getsockopt.optname == SO_TYPE) {
            int kind = packet_getsocktype(pcb) == PACKET_TYPE_DGRAM ? SOCK_DGRAM
                                                                    : SOCK_RAW;
            auto result = copy_out(getsockopt.id.pid, getsockopt.optval, kind);
            if (!result) return (tl::make_unexpected(result.error()));
            return (kind);
        }
        return (do_sock_getsockopt(reinterpret_cast<const ip_pcb*>(pcb),
                                   getsockopt));
    case SOL_PACKET:
        if (getsockopt.optname == PACKET_STATISTICS) {
            auto stats = tpacket_stats{.tp_packets = packet_stat_total(pcb),
                                       .tp_drops = packet_stat_drops(pcb)};
            auto result = copy_out(getsockopt.id.pid, getsockopt.optval, stats);
            if (!result) return (tl::make_unexpected(result.error()));
            return (sizeof(stats));
        }
        [[fallthrough]];
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

tl::expected<void, int>
packet_socket::do_setsockopt(packet_pcb* pcb,
                             const api::request_setsockopt& setsockopt)
{
    switch (setsockopt.level) {
    case SOL_SOCKET:
        return (do_sock_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case SOL_PACKET:
        switch (setsockopt.optname) {
        case PACKET_ADD_MEMBERSHIP:
        case PACKET_DROP_MEMBERSHIP:
            /*
             * Our packet sockets are promiscuous by default, so implementing
             * these options aren't strictly necessary. Just silently ignore
             * them for now.
             */
            return {};
        default:
            return (tl::make_unexpected(ENOPROTOOPT));
        }
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

packet_socket::on_request_reply
packet_socket::on_request(const api::request_bind& bind, const packet_init&)
{
    auto result = do_packet_bind(m_pcb, bind);
    if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
    return {api::reply_success(), packet_bound{}};
}

packet_socket::on_request_reply
packet_socket::on_request(const api::request_getsockname& getsockname,
                          const packet_bound&)
{
    auto sll = sockaddr_ll{};
    socklen_t slength = sizeof(sll);

    auto error = packet_getsockname(m_pcb, &sll);
    if (error) {
        return {tl::make_unexpected(err_to_errno(error)), std::nullopt};
    }

    auto result = copy_out(getsockname.id.pid,
                           getsockname.name,
                           std::addressof(sll),
                           std::min(getsockname.namelen, slength));
    if (!result) { return {tl::make_unexpected(result.error()), std::nullopt}; }

    return {api::reply_socklen{slength}, std::nullopt};
}

} // namespace openperf::socket::server
