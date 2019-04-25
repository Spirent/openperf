#include <cerrno>
#include <cstring>
#include <memory>
#include <optional>
#include <unistd.h>
#include <unordered_map>
#include <variant>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>

#include "socket/api.h"
#include "socket/dgram_channel.h"
#include "socket/stream_channel.h"
#include "socket/uuid.h"
#include "socket/server/api_handler.h"
#include "socket/server/api_server.h"
#include "core/icp_core.h"

namespace icp {
namespace socket {
namespace api {

using api_handler = icp::socket::server::api_handler;

static bool unlink_stale_files = false;
static std::string prefix_opt;

static __attribute__((const)) uint64_t align_up(uint64_t x, uint64_t align)
{
    return ((x + align - 1) & ~(align - 1));
}

static icp::memory::shared_segment create_shared_memory(size_t size)
{
    auto shared_segment_name = (prefix_opt.length() > 0
                                ? std::string(api::key) + ".memory." + prefix_opt
                                : std::string(api::key) + ".memory");
    if (unlink_stale_files) {
        if ((shm_unlink(shared_segment_name.data()) < 0) && (errno != ENOENT)) {
            throw std::runtime_error("Could not remove shared memory segment "
                                     + std::string(shared_segment_name) + ": " + strerror(errno));
        }
    }

    auto impl_size = align_up(sizeof(socket::server::allocator), 64);
    auto segment = icp::memory::shared_segment(shared_segment_name.data(),
                                               size, true);

    /* Construct our allocator in our shared memory segment */
    new (segment.base()) socket::server::allocator(
        reinterpret_cast<uintptr_t>(segment.base()) + impl_size,
        size - impl_size);

    return (segment);
}

socket::server::allocator* server::allocator() const
{
    return (reinterpret_cast<socket::server::allocator*>(m_shm.base()));
}

static icp::socket::unix_socket create_unix_socket(const std::string_view path, int type)
{
    if (unlink_stale_files) {
        if ((unlink(path.data()) < 0) && (errno != ENOENT)) {
            throw std::runtime_error("Could not remove shared unix domain socket "
                                     + std::string(path) + ": " + strerror(errno));
        }
    }

    auto socket = icp::socket::unix_socket(path, type);

    return (socket);
}

extern "C" {
static int handle_api_accept(const struct icp_event_data *data __attribute__((unused)),
                             void *arg)
{
    auto server = reinterpret_cast<icp::socket::api::server*>(arg);
    return (server->handle_api_accept_requests());
}

static int handle_api_init(const struct icp_event_data *data,
                           void *arg)
{
    auto server = reinterpret_cast<icp::socket::api::server*>(arg);
    return (server->handle_api_init_requests(data));
}

static int handle_api_read(const struct icp_event_data *data,
                           void *arg)
{
    auto server = reinterpret_cast<icp::socket::api::server*>(arg);
    return (server->handle_api_read_requests(data));
}

static int handle_api_delete(const struct icp_event_data *data,
                             void *arg)
{
    auto server = reinterpret_cast<icp::socket::api::server*>(arg);
    return (server->handle_api_delete_requests(data));
}

static int close_fd(const struct icp_event_data *data,
                    void *arg __attribute__((unused)))
{
    close(data->fd);
    return (0);
}

int api_server_option_handler(int opt, const char *opt_arg __attribute__((unused)))
{
    if (icp_options_hash_long("force-unlink") == opt) {
        unlink_stale_files = true;
        return (0);
    }
    if (icp_options_hash_long("prefix") == opt) {
        prefix_opt = opt_arg;
        return (0);
    }
    return (-EINVAL);
}

const char* api_server_options_prefix_option_get(void)
{
    return (prefix_opt.c_str());
}

}

server::server(icp::core::event_loop& loop)
    : m_sock(create_unix_socket((prefix_opt.length() > 0) ? (api::server_socket() +
                                "." + prefix_opt) : api::server_socket(),
                                api::socket_type))
    , m_shm(create_shared_memory(shm_size))
    , m_loop(loop)
{
    /* Put socket in the listen state and wait for connections */
    if (listen(m_sock.get(), 8) == -1) {
        throw std::runtime_error("Could not listen to socket: "
                                 + std::string(strerror(errno)));
    }

    struct icp_event_callbacks callbacks = {
        .on_read = handle_api_accept
    };
    m_loop.add(m_sock.get(), &callbacks, this);
}

server::~server() {}

int server::handle_api_accept_requests()
{
    /**
     * Every connection represents a new thread that wants to access our
     * sockets.
     */
    for (;;) {
        sockaddr_un client;
        socklen_t client_length = sizeof(client);
        auto fd = accept(m_sock.get(),
                         reinterpret_cast<sockaddr*>(&client),
                         &client_length);
        if (fd == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                ICP_LOG(ICP_LOG_ERROR, "Could not accept socket: %s\n",
                        strerror(errno));
            }
            break;
        }

        /* Handle requests on the new fd */
        struct icp_event_callbacks callbacks = {
            .on_read  = handle_api_init,
            .on_close = close_fd
        };
        m_loop.add(fd, &callbacks, this);
    }

    return (0);
}

int server::handle_api_init_requests(const struct icp_event_data *data)
{
    for (;;) {
        api::request_msg request;
        auto ret = recv(data->fd, &request, sizeof(request), 0);
        if (ret == -1) {
            break;
        }

        if (!std::holds_alternative<api::request_init>(request)) {
            ICP_LOG(ICP_LOG_ERROR, "Received unexpected message during "
                    "client init phase");
            api::reply_msg reply = tl::make_unexpected(EINVAL);
            send(data->fd, &reply, sizeof(reply), 0);
            continue;
        }

        /* We'll return this; eventually */
        api::reply_msg reply;

        /*
         * We need to examine our init message to find out what pid
         * is contacting us so that we can setup a proper socket
         * handler.
         */
        auto init = std::get<api::request_init>(request);
        if (m_handlers.find(init.pid) == m_handlers.end()) {
            ICP_LOG(ICP_LOG_INFO, "New connection received from pid %d, %s\n",
                    init.pid, to_string(init.tid).c_str());
            m_handlers.emplace(init.pid,
                               std::make_unique<api_handler>(m_loop, m_shm.base(),
                                                             *(allocator()),
                                                             init.pid));
            auto shm_info = api::shared_memory_descriptor{
                .size = m_shm.size()
            };
            strncpy(shm_info.name, m_shm.name().data(),
                    api::shared_memory_name_length);
            reply = api::reply_init{
                .pid = getpid(),
                .shm_info = std::make_optional(shm_info)
            };
        } else {
            /*
             * We have already responded to an init request for this pid.
             * Since the client should already have the shared memory
             * data, don't return it again.
             */
            reply = api::reply_init{
                .pid = getpid(),
                .shm_info = std::nullopt
            };
        }

        /*
         * XXX: We are going to replace our callback with one that can
         * actually handle socket requests.  To do that, we dup our existing
         * fd, delete our old callback, and add our new one.  Our event
         * loop can't handle argument updates for callbacks, so we just do
         * this shuffle instead.  Since we only expect to do this once per
         * client thread, this shouldn't be a common occurrence.
         */
        auto newfd = dup(data->fd);
        if (newfd == -1) {
            ICP_LOG(ICP_LOG_ERROR, "Could not duplicate descriptor: %s\n",
                    strerror(errno));
            reply = tl::make_unexpected(errno);
            send(data->fd, &reply, sizeof(reply), 0);
            continue;
        }
        m_loop.del(data->fd);

        m_pids.emplace(newfd, init.pid);

        struct icp_event_callbacks callbacks = {
            .on_read   = handle_api_read,
            .on_delete = handle_api_delete,
            .on_close  = close_fd
        };

        m_loop.add(newfd, &callbacks, this);
        send(newfd, &reply, sizeof(reply), 0);
    }

    return (0);
}

int server::handle_api_read_requests(const struct icp_event_data *data)
{
    /* Find the appropriate handler for this fd */
    auto pid_result = m_pids.find(data->fd);
    if (pid_result == m_pids.end()) {
        ICP_LOG(ICP_LOG_ERROR, "Could not locate pid for fd = %d\n", data->fd);
        return (-1);
    }

    auto pid = pid_result->second;
    auto handler_result = m_handlers.find(pid);
    if (handler_result == m_handlers.end()) {
        ICP_LOG(ICP_LOG_ERROR, "Could not locate handler for pid = %d\n", pid);
        return (-1);
    }

    auto& handler = handler_result->second;
    return (handler->handle_requests(data));
}

int server::handle_api_delete_requests(const struct icp_event_data *data)
{
    auto pid_result = m_pids.find(data->fd);
    if (pid_result == m_pids.end()) {
        ICP_LOG(ICP_LOG_ERROR, "Could not locate pid for fd = %d\n", data->fd);
        return (-1);
    }
    auto pid = pid_result->second;

    /* Remove fd, pid pair from our map */
    m_pids.erase(pid_result);

    /* Check and see if this was the last fd for this pid. */
    bool found = false;
    for (auto& kv : m_pids) {
        if (pid == kv.second) {
            found = true;
            break;
        }
    }

    if (!found) {
        /* All connections for this handler are gone; delete it */
        ICP_LOG(ICP_LOG_INFO, "All connections from pid %d are gone; cleaning up\n", pid);
        m_handlers.erase(pid);
    }

    return (0);
}

}
}
}
