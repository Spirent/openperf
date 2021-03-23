#include "api/api_route_handler.hpp"
#include "api/api_utils.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "framework/utils/overloaded_visitor.hpp"
#include "socket/server/lwip_utils.hpp"
#include "socket/server/pcb_utils.hpp"

#include "lwip/tcpbase.h"
#include <arpa/inet.h>

#include "swagger/v1/model/SocketStats.h"

using namespace swagger::v1::model;

namespace openperf::socket::api {

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Pistache::Rest::Router& router);

    using request_type = Pistache::Rest::Request;
    using response_type = Pistache::Http::ResponseWriter;

    void list_sockets(const request_type& request, response_type response);
};

handler::handler(void* context, Pistache::Rest::Router& router)
{
    using namespace Pistache::Rest::Routes;

    Get(router, "/sockets", bind(&handler::list_sockets, this));
}

using namespace Pistache;

static std::string json_error(int code, std::string_view message)
{
    return (
        nlohmann::json({{"code", code}, {"message", message.data()}}).dump());
}

std::unique_ptr<SocketStats>
make_swagger_socket_stats(const socket::server::socket_pcb_stats& src)
{
    auto dst = std::make_unique<SocketStats>();

    if (src.id.pid != 0 || src.id.sid != 0) {
        dst->setPid(src.id.pid);
        dst->setSid(src.id.sid);
    }

    dst->setRxqBytes(src.channel_stats.rxq_len);
    dst->setTxqBytes(src.channel_stats.txq_len);

    const char* ip_str;
    char ip_buf[INET_ADDRSTRLEN];

    std::visit(utils::overloaded_visitor(
                   [&](const std::monostate&) { dst->setProtocol("raw"); },
                   [&](const socket::server::tcp_pcb_stats& stats) {
                       dst->setProtocol("tcp");
                       ip_str = inet_ntop(stats.ip_address_family,
                                          stats.local_ip.data(),
                                          ip_buf,
                                          sizeof(ip_buf))
                                    ? ip_buf
                                    : "<?>";
                       dst->setLocalIpAddress(ip_str);
                       ip_str = inet_ntop(stats.ip_address_family,
                                          stats.remote_ip.data(),
                                          ip_buf,
                                          sizeof(ip_buf))
                                    ? ip_buf
                                    : "<?>";
                       dst->setRemoteIpAddress(ip_str);
                       dst->setLocalPort(stats.local_port);
                       dst->setRemotePort(stats.remote_port);
                       dst->setSendQueueLength(stats.snd_queuelen);
                       dst->setState(tcp_debug_state_str(
                           static_cast<tcp_state>(stats.state)));
                   },
                   [&](const socket::server::udp_pcb_stats& stats) {
                       dst->setProtocol("udp");
                       ip_str = inet_ntop(stats.ip_address_family,
                                          stats.local_ip.data(),
                                          ip_buf,
                                          sizeof(ip_buf))
                                    ? ip_buf
                                    : "<?>";
                       dst->setLocalIpAddress(ip_str);
                       ip_str = inet_ntop(stats.ip_address_family,
                                          stats.remote_ip.data(),
                                          ip_buf,
                                          sizeof(ip_buf))
                                    ? ip_buf
                                    : "<?>";
                       dst->setRemoteIpAddress(ip_str);
                       dst->setLocalPort(stats.local_port);
                       dst->setRemotePort(stats.remote_port);
                   }),
               src.pcb_stats);

    return (dst);
}

void handler::list_sockets(const request_type& request, response_type response)
{
    std::list<socket::server::socket_pcb_stats> socket_stats;
    // PCBs/Sockets need to be accessed from the TCP thread
    auto result = socket::server::do_tcpip_call([&socket_stats]() {
        socket_stats = socket::server::get_all_socket_pcb_stats();
        return ERR_OK;
    });
    if (result != ERR_OK) {
        response.send(
            Http::Code::Internal_Server_Error,
            json_error(static_cast<int>(result), lwip_strerr(result)));
        return;
    }
    auto sockets = nlohmann::json::array();
    std::transform(socket_stats.begin(),
                   socket_stats.end(),
                   std::back_inserter(sockets),
                   [](const auto& stats) {
                       auto swagger_stats = make_swagger_socket_stats(stats);
                       return (swagger_stats->toJson());
                   });

    openperf::api::utils::send_chunked_response(
        std::move(response), Http::Code::Ok, sockets);
}

} // namespace openperf::socket::api
