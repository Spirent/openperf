#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <unistd.h>
#include <unordered_map>
#include <variant>

#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>

#include "socket/api.h"
#include "socket/dgram_channel.h"
#include "socket/stream_channel.h"
#include "socket/server/api_handler.h"
#include "socket/server/api_server.h"
#include "core/icp_core.h"
#include "core/icp_uuid.h"
#include "icp_config_file.h"

namespace icp {
namespace socket {
namespace api {

using api_handler = icp::socket::server::api_handler;

static constexpr std::string_view yama_file = "/proc/sys/kernel/yama/ptrace_scope";

static __attribute__((const)) uint64_t align_up(uint64_t x, uint64_t align)
{
    return ((x + align - 1) & ~(align - 1));
}

bool unlink_stale_files() {
    auto result = config::file::icp_config_get_param("modules.socket.force-unlink");
    if (!result) { return false; }

    return result.value().has_value();
}

std::string prefix_option() {
    auto result = config::file::icp_config_get_param("modules.socket.prefix");

    if ((!result) || (!result.value().has_value())) { return std::string(); }

    try {
        return std::any_cast<std::string>(result.value());
    }
    catch (...) {
        return std::string();
    }
}

static icp::memory::shared_segment create_shared_memory(size_t size)
{
    auto prefix_name = prefix_option();

    auto shared_segment_name = (prefix_name.length() > 0
                                ? std::string(api::key) + ".memory." + prefix_name
                                : std::string(api::key) + ".memory");
    if (unlink_stale_files()) {
        if ((shm_unlink(shared_segment_name.data()) < 0) && (errno != ENOENT)) {
            throw std::runtime_error("Could not remove shared memory segment "
                                     + std::string(shared_segment_name) + ": " + strerror(errno));
        }

        ICP_LOG(ICP_LOG_DEBUG, "Unlinking stale shared memory at: %s", shared_segment_name.c_str());
    }

    auto impl_size = align_up(sizeof(socket::server::allocator), 64);
    auto segment = icp::memory::shared_segment(shared_segment_name.data(),
                                               size, true);

    /* Construct our allocator in our shared memory segment */
    new (segment.base()) socket::server::allocator(
        reinterpret_cast<uintptr_t>(segment.base()) + impl_size,
        size - impl_size);

    ICP_LOG(ICP_LOG_DEBUG, "Created shared memory at: %s, with size %lu", shared_segment_name.c_str(), size);

    return (segment);
}

socket::server::allocator* server::allocator() const
{
    return (reinterpret_cast<socket::server::allocator*>(m_shm.base()));
}

static icp::socket::unix_socket create_unix_socket(const std::string_view path, int type)
{
    // Is there a prefix for the path?
    std::string full_path(path);
    if (auto prefix_name = prefix_option();
        prefix_name.length() > 0) {
        full_path += "." + prefix_name;
    }

    if (unlink_stale_files()) {
        if ((unlink(full_path.c_str()) < 0) && (errno != ENOENT)) {
            throw std::runtime_error("Could not remove shared unix domain socket "
                                     + full_path + ": " + strerror(errno));
        }
        ICP_LOG(ICP_LOG_DEBUG, "Unlinking stale server socket at: %s", full_path.c_str());
    }

    auto socket = icp::socket::unix_socket(full_path, type);

    ICP_LOG(ICP_LOG_DEBUG, "Created unix socket at: %s", full_path.c_str());

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

static void update_yama_related_process_settings()
{
    if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0) {
        throw std::runtime_error("Could not set dumpable: "
                                 + std::string(strerror(errno)));
    }

    std::ifstream f (yama_file.data(), std::ifstream::in);
    if (f.is_open()) {
        char c;
        if (f.read(&c, 1)) {
            switch (c) {
            case '1':
                if (prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0) < 0) {
                    throw std::runtime_error("Could not set ptracer_any: "
                                             + std::string(strerror(errno)));
                }
                break;
            case '2':
                ICP_LOG(ICP_LOG_WARNING, "inception and its clients need to run as root or \
                        change contents of %s to 1\n", yama_file.data());
                break;
            case '3':
                ICP_LOG(ICP_LOG_WARNING, "inception requires the contents of %s to be 0, 1, or 2\n",
                        yama_file.data());
                break;
            default:
                break;
            }
        } else {
            throw std::runtime_error("Could not read from file: " + std::string(strerror(errno)));
        }
    }
}

const char* api_server_options_prefix_option_get(void)
{
    static std::string prefix_opt = prefix_option();
    return (prefix_opt.c_str());
}

}

server::server(icp::core::event_loop& loop)
    : m_sock(create_unix_socket(api::server_socket(),
                                api::socket_type))
    , m_shm(create_shared_memory(shm_size))
    , m_loop(loop)
{
    /* Put socket in the listen state and wait for connections */
    if (listen(m_sock.get(), 8) == -1) {
        throw std::runtime_error("Could not listen to socket: "
                                 + std::string(strerror(errno)));
    }

    update_yama_related_process_settings();

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
