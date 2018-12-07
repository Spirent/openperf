#ifndef _ICP_SOCKET_API_H_
#define _ICP_SOCKET_API_H_

#include <string>
#include <optional>
#include <variant>
#include <limits.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "tl/expected.hpp"

#include "socket/spsc_queue.h"
#include "socket/uuid.h"

namespace icp {
namespace sock {

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

namespace api {

constexpr int    socket_fd_offset = std::numeric_limits<int>::max() / 2;
constexpr size_t max_sockets = 512;
constexpr size_t socket_queue_length = 32;
constexpr size_t shared_memory_name_length = 32;

typedef spsc_queue<iovec, socket_queue_length> socket_queue;

struct socket_queue_pair {
    socket_queue txq;
    socket_queue rxq;
};

struct socket_fd_pair {
    int client_fd;
    int server_fd;
} __attribute__((packed));

struct shared_memory_descriptor {
    char   name[shared_memory_name_length];
    void  *base;
    size_t size;
};

struct reply_init {
    std::optional<shared_memory_descriptor> shm_info;
};

struct reply_socket {
    socket_queue_pair* queue_pair;
    socket_fd_pair fd_pair;
    int sockid;
};

struct reply_success {};

struct request_init {
    pid_t pid;
    uuid  tid;
};

struct request_accept {
    int sockid;
    const struct sockaddr *addr;
    socklen_t addrlen;
};

struct request_bind {
    int sockid;
    const struct sockaddr *name;
    socklen_t namelen;
};

struct request_shutdown {
    int sockid;
    int how;
};

struct request_getpeername {
    int sockid;
    struct sockaddr *name;
    socklen_t namelen;
};

struct request_getsockname {
    int sockid;
    struct sockaddr *name;
    socklen_t namelen;
};

struct request_getsockopt {
    int sockid;
    int level;
    int optname;
    void *optval;
    socklen_t optlen;
};

struct request_setsockopt {
    int sockid;
    int level;
    int optname;
    const void *optval;
    socklen_t optlen;
};

struct request_close {
    int sockid;
};

struct request_connect {
    int sockid;
    const struct sockaddr *name;
    socklen_t namelen;
};

struct request_listen {
    int sockid;
    int backlog;
};

struct request_socket {
    int sockid;
    int domain;
    int type;
    int protocol;
};

typedef std::variant<request_init,
                     request_accept,
                     request_bind,
                     request_shutdown,
                     request_getpeername,
                     request_getsockname,
                     request_getsockopt,
                     request_setsockopt,
                     request_close,
                     request_connect,
                     request_listen,
                     request_socket> request_msg;

typedef std::variant<reply_success,
                     reply_socket,
                     reply_init> reply_ok;

typedef tl::expected<reply_ok, int> reply_msg;  /* either the reply or an error code */

constexpr std::string_view key = "com.spirent.inception";

constexpr int socket_type = SOCK_SEQPACKET;

std::string client_socket(const std::string_view id);

std::string server_socket();

std::optional<socket_fd_pair> get_message_fds(const api::reply_msg&);
void set_message_fds(api::reply_msg&, socket_fd_pair& fd_pair);

}
}
}

#endif /* _ICP_SOCKET_API_H_ */
