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
namespace socket {

struct dgram_channel;
struct stream_channel;

namespace api {

constexpr size_t max_sockets = 512;
constexpr size_t socket_queue_length = 32;
constexpr size_t shared_memory_name_length = 32;

typedef std::variant<dgram_channel*, stream_channel*> io_channel;

struct socket_fd_pair {
    int client_fd;
    int server_fd;
} __attribute__((packed));

struct shared_memory_descriptor {
    char   name[shared_memory_name_length];
    void  *base;
    size_t size;
};

union socket_id {
    struct {
        pid_t pid;
        uint32_t sid;
    };
    uint64_t uint64;

    bool operator==(const socket_id& other) const
    {
        return (uint64 == other.uint64);
    }
};

struct reply_init {
    pid_t pid;
    std::optional<shared_memory_descriptor> shm_info;
};

struct reply_accept {
    socket_id id;
    io_channel channel;
    socket_fd_pair fd_pair;
    socklen_t addrlen;
};

struct reply_socket {
    socket_id id;
    io_channel channel;
    socket_fd_pair fd_pair;
};

struct reply_socklen {
    socklen_t length;
};

struct reply_success {};

struct request_init {
    pid_t pid;
    uuid  tid;
};

struct request_accept {
    socket_id id;
    int flags;
    struct sockaddr *addr;
    socklen_t addrlen;
};

struct request_bind {
    socket_id id;
    const struct sockaddr *name;
    socklen_t namelen;
};

struct request_shutdown {
    socket_id id;
    int how;
};

struct request_getpeername {
    socket_id id;
    struct sockaddr *name;
    socklen_t namelen;
};

struct request_getsockname {
    socket_id id;
    struct sockaddr *name;
    socklen_t namelen;
};

struct request_getsockopt {
    socket_id id;
    int level;
    int optname;
    void *optval;
    socklen_t optlen;
};

struct request_setsockopt {
    socket_id id;
    int level;
    int optname;
    const void *optval;
    socklen_t optlen;
};

struct request_close {
    socket_id id;
};

struct request_connect {
    socket_id id;
    const struct sockaddr *name;
    socklen_t namelen;
};

struct request_listen {
    socket_id id;
    int backlog;
};

struct request_socket {
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

typedef std::variant<reply_init,
                     reply_accept,
                     reply_socket,
                     reply_socklen,
                     reply_success> reply_ok;

typedef tl::expected<reply_ok, int> reply_msg;  /* either the reply or an error code */

constexpr std::string_view key = "com.spirent.inception";

constexpr int socket_type = SOCK_SEQPACKET;

std::string client_socket(const std::string_view id);

std::string server_socket();

std::string& get_prefix_opt();

std::optional<socket_fd_pair> get_message_fds(const api::reply_msg&);
void set_message_fds(api::reply_msg&, const socket_fd_pair& fd_pair);

}
}
}

namespace std {

template <>
struct hash<icp::socket::api::socket_id>
{
    size_t operator()(const icp::socket::api::socket_id& id) const noexcept
    {
        return (std::hash<uint64_t>{}(id.uint64));
    }
};

}

#endif /* _ICP_SOCKET_API_H_ */
