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

namespace icp::socket::api {

using api_handler = icp::socket::server::api_handler;

static constexpr std::string_view yama_file = "/proc/sys/kernel/yama/ptrace_scope";

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

server::server(void* context)
    : m_context(context)
    , m_sock(create_unix_socket((prefix_opt.length() > 0
                                 ? (api::server_socket() + "." + prefix_opt)
                                 : api::server_socket()),
                                api::socket_type))
    , m_shm(create_shared_memory(shm_size))
{
    /* Put socket in the listen state and wait for connections */
    if (listen(m_sock.get(), 8) == -1) {
        throw std::runtime_error("Could not listen to socket: "
                                 + std::string(strerror(errno)));
    }

    update_yama_related_process_settings();

}

server::~server() {}

int server::start()
{
    auto client = packetio::internal::api::client(m_context);

    assert(m_task.empty());

    using namespace std::placeholders;
    auto result = client.add_task("icp::socket::server api handler",
                                  packetio::workers::context::STACK,
                                  m_sock.get(),
                                  std::bind(&server::handle_api_accept, this, _1, _2),
                                  nullptr);
    if (!result) {
        return (result.error());
    }

    m_task = *result;
    return (0);
}

void server::stop()
{
    assert(!m_task.empty());

    auto client = packetio::internal::api::client(m_context);
    client.del_task(m_task);
    m_task.clear();
}

int server::handle_api_accept(event_loop& loop, std::any)
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

        using namespace std::placeholders;
        if (!loop.add_callback("icp::socket::server::handle_api_init for fd = " + std::to_string(fd),
                               fd,
                               std::bind(&server::handle_api_init, this, _1, _2),
                               fd)) {
            ICP_LOG(ICP_LOG_ERROR, "Failed to add api init callback for fd = %d\n", fd);
            return (-1);
        }
    }

    return (0);
}

int server::handle_api_init(event_loop& loop, std::any arg)
{
    int fd = std::any_cast<int>(arg);

    for (;;) {
        api::request_msg request;
        auto ret = recv(fd, &request, sizeof(request), 0);
        if (ret == -1) {
            break;
        }

        if (!std::holds_alternative<api::request_init>(request)) {
            ICP_LOG(ICP_LOG_ERROR, "Received unexpected message during "
                    "client init phase");
            api::reply_msg reply = tl::make_unexpected(EINVAL);
            send(fd, &reply, sizeof(reply), 0);
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
                               std::make_unique<api_handler>(loop, m_shm.base(),
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
        auto newfd = dup(fd);
        if (newfd == -1) {
            ICP_LOG(ICP_LOG_ERROR, "Could not duplicate descriptor: %s\n",
                    strerror(errno));
            reply = tl::make_unexpected(errno);
            send(fd, &reply, sizeof(reply), 0);
            continue;
        }
        loop.del_callback(fd);

        m_pids.emplace(newfd, init.pid);

        using namespace std::placeholders;
        loop.add_callback("icp::socket::server::handle_api_read for fd = " + std::to_string(newfd),
                          newfd,
                          std::bind(&server::handle_api_read, this, _1, _2),
                          newfd);
        send(newfd, &reply, sizeof(reply), 0);
    }

    return (-1);
}

int server::handle_api_read(event_loop&, std::any arg)
{
    int fd = std::any_cast<int>(arg);

    /* Find the appropriate handler for this fd */
    auto pid_result = m_pids.find(fd);
    if (pid_result == m_pids.end()) {
        ICP_LOG(ICP_LOG_ERROR, "Could not locate pid for fd = %d\n", fd);
        return (handle_api_delete(fd));
    }

    auto pid = pid_result->second;
    auto handler_result = m_handlers.find(pid);
    if (handler_result == m_handlers.end()) {
        ICP_LOG(ICP_LOG_ERROR, "Could not locate handler for pid = %d\n", pid);
        return (handle_api_delete(fd));
    }

    auto& handler = handler_result->second;
    return (handler->handle_requests(fd) || handle_api_delete(fd));
}

int server::handle_api_delete(int fd)
{
    auto pid_result = m_pids.find(fd);
    if (pid_result == m_pids.end()) {
        ICP_LOG(ICP_LOG_ERROR, "Could not locate pid for fd = %d\n", fd);
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

    close(fd);
    return (-1);
}

}

extern "C" {

int api_server_option_handler(int opt, const char *opt_arg __attribute__((unused)))
{
    if (icp_options_hash_long("force-unlink") == opt) {
        icp::socket::api::unlink_stale_files = true;
        return (0);
    }
    if (icp_options_hash_long("prefix") == opt) {
        icp::socket::api::prefix_opt = opt_arg;
        return (0);
    }
    return (-EINVAL);
}

const char* api_server_options_prefix_option_get(void)
{
    return (icp::socket::api::prefix_opt.c_str());
}

}
