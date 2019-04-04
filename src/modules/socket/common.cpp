#include "socket/api.h"

namespace icp {
namespace socket {
namespace api {

static std::string prefix_opt;

static int handle_options(int opt, const char *opt_arg)
{
    if (opt != 'm' || opt_arg == nullptr) {
        return (-EINVAL);
    }
    prefix_opt = opt_arg;
    return (0);
}

std::string& get_prefix_opt()
{
    return (prefix_opt);
}

/**
 * This struct is magic.  Use templates and parameter packing to provide
 * some syntactic sugar for creating visitor objects for std::visit.
 */
template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

std::string client_socket(const std::string_view id)
{
    if (auto envp = std::getenv("ICP_PREFIX"); envp != nullptr) {
        return (std::string("/tmp/.").append(key).append("/").append(envp).append("/client.").append(id));
    }
    return (std::string("/tmp/.").append(key).append("/client.").append(id));
}

std::string server_socket()
{
    if (prefix_opt.length() != 0) {
        return (std::string("/tmp/.").append(key).append("/" + prefix_opt).append("/server"));
    } else if (auto envp = std::getenv("ICP_PREFIX"); envp != nullptr) {
        return (std::string("/tmp/.").append(key).append("/").append(envp).append("/server"));
    }
    return (std::string("/tmp/.").append(key).append("/server"));
}

std::optional<socket_fd_pair> get_message_fds(const reply_msg& reply)
{
    if (!reply) {
        return (std::nullopt);
    }

    return (std::visit(overloaded_visitor(
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
                           }),
                       *reply));
}

void set_message_fds(reply_msg& reply, const socket_fd_pair& fd_pair)
{
    if (!reply) {
        return;
    }

    std::visit(overloaded_visitor(
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
                   }),
               *reply);
}

}
}
}

extern "C" {

int socket_option_handler(int opt, const char *opt_arg)
{
    return (icp::socket::api::handle_options(opt, opt_arg));
}
}
