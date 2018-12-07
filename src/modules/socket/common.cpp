#include "socket/socket_api.h"

namespace icp {
namespace sock {
namespace api {

std::string client_socket(const std::string_view id)
{
    return (std::string("/tmp/.").append(key).append("/client.").append(id));
}

std::string server_socket()
{
    return (std::string("/tmp/.").append(key).append("/server"));
}

std::optional<socket_fd_pair> get_message_fds(const reply_msg& reply)
{
    if (!reply) {
        return (std::nullopt);
    }

    return (std::holds_alternative<reply_socket>(*reply)
            ? std::make_optional(std::get<reply_socket>(*reply).fd_pair)
            : std::nullopt);
}

void set_message_fds(reply_msg& reply, socket_fd_pair& fd_pair)
{
    if (!reply) {
        return;
    }

    if (std::holds_alternative<reply_socket>(*reply)) {
        std::get<reply_socket>(*reply).fd_pair = fd_pair;
    }
}

}
}
}
