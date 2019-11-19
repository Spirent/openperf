#include "socket/api.h"
#include "utils/overloaded_visitor.h"

namespace icp {
namespace socket {
namespace api {

std::string client_socket(const std::string_view id)
{
    if (auto envp = std::getenv("ICP_PREFIX"); envp != nullptr) {
        return (std::string("/tmp/.").append(key).append("/client.").append(id).append("." +
                std::string(envp)));
    }
    return (std::string("/tmp/.").append(key).append("/client.").append(id));
}

std::string server_socket()
{
    return (std::string("/tmp/.").append(key).append("/server"));
}

std::optional<api::socket_fd_pair> get_message_fds(const api::reply_msg& reply)
{
    if (!reply) {
        return (std::nullopt);
    }

    return (std::visit(utils::overloaded_visitor(
                           [](const api::reply_accept& accept) -> std::optional<socket_fd_pair> {
                               return (std::make_optional(accept.fd_pair));
                           },
                           [](const api::reply_init&) -> std::optional<socket_fd_pair> {
                               return (std::nullopt);
                           },
                           [](const api::reply_socket& socket) -> std::optional<socket_fd_pair> {
                               return (std::make_optional(socket.fd_pair));
                           },
                           [](const api::reply_socklen&) -> std::optional<socket_fd_pair> {
                               return (std::nullopt);
                           },
                           [](const api::reply_success&) -> std::optional<socket_fd_pair> {
                               return (std::nullopt);
                           },
                           [](const api::reply_working&) -> std::optional<socket_fd_pair> {
                               return (std::nullopt);
                           }),
                       *reply));
}

void set_message_fds(api::reply_msg& reply, const api::socket_fd_pair& fd_pair)
{
    if (!reply) {
        return;
    }

    std::visit(utils::overloaded_visitor(
                   [&](api::reply_accept& accept) {
                       accept.fd_pair = fd_pair;
                   },
                   [](api::reply_init&) {
                       ;
                   },
                   [&](api::reply_socket& socket) {
                       socket.fd_pair = fd_pair;
                   },
                   [](api::reply_socklen&) {
                       ;
                   },
                   [](api::reply_success&) {
                       ;
                   },
                   [](api::reply_working&) {
                       ;
                   }),
               *reply);
}

api::io_channel_ptr to_pointer(api::io_channel_offset offset, const void* base)
{
    return (std::visit(utils::overloaded_visitor(
                           [&](dgram_channel_offset dgram) -> io_channel_ptr {
                               return (reinterpret_cast<dgram_channel*>(
                                           reinterpret_cast<intptr_t>(base) + dgram.offset));
                           },
                           [&](stream_channel_offset stream) -> io_channel_ptr {
                                return (reinterpret_cast<stream_channel*>(
                                            reinterpret_cast<intptr_t>(base) + stream.offset));
                           }),
                       std::move(offset)));
}

}
}
}
