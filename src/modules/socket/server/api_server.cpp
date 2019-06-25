#include <cerrno>
#include <cstring>
#include <fstream>
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

namespace icp::socket::api {

using api_handler = icp::socket::server::api_handler;

static constexpr std::string_view yama_file = "/proc/sys/kernel/yama/ptrace_scope";

static __attribute__((const)) uint64_t align_up(uint64_t x, uint64_t align)
{
    return ((x + align - 1) & ~(align - 1));
}

bool unlink_stale_files() {
    auto result = config::file::icp_config_get_param<ICP_OPTION_TYPE_NONE>("modules.socket.force-unlink");

    return (result.value_or(false));
}

std::string prefix_option() {
    auto result = config::file::icp_config_get_param<ICP_OPTION_TYPE_STRING>("modules.socket.prefix");

    return (result.value_or(std::string()));
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
    , m_sock(create_unix_socket(api::server_socket(),
                                SOCK_NONBLOCK | api::socket_type))
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
    auto result = client.add_task(packetio::workers::context::STACK,
                                  "socket API",
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
        if (!loop.add_callback("socket API for fd = " + std::to_string(fd),
                               fd,
                               std::bind(&server::handle_api_client, this, _1, _2),
                               std::bind(&server::handle_api_error, this, _1),
                               fd)) {
            ICP_LOG(ICP_LOG_ERROR, "Failed to add socket API callback for fd = %d\n", fd);
        }
    }

    return (0);
}

int server::handle_api_client(event_loop& loop, std::any arg)
{
    auto fd = std::any_cast<int>(arg);

    /*
     * Look for the pid for this fd.  If we don't have it, then we need to
     * do some initialization.
     */
    auto pid_result = m_pids.find(fd);
    return (pid_result == m_pids.end()
            ? do_client_init(loop, fd)
            : do_client_read(pid_result->second, fd));
}

int server::do_client_init(event_loop& loop, int fd)
{
    for (;;) {
        api::request_msg request;
        auto ret = recv(fd, &request, sizeof(request), MSG_DONTWAIT);
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

        if (send(fd, &reply, sizeof(reply), 0) > 0) {
            ICP_LOG(ICP_LOG_DEBUG, "Initialized client from pid %d, %s\n",
                    init.pid, to_string(init.tid).c_str());

            /* Add this client to our pid map */
            m_pids.emplace(fd, init.pid);
        }
    }

    return (0);
}

int server::do_client_read(pid_t pid, int fd)
{
    /* Find the handler for this pid and dispatch to it */
    auto handler_result = m_handlers.find(pid);
    if (handler_result == m_handlers.end()) {
        ICP_LOG(ICP_LOG_ERROR, "Could not locate handler for pid = %d\n", pid);
        return (-1);
    }

    auto& handler = handler_result->second;
    return (handler->handle_requests(fd));
}

void server::handle_api_error(std::any arg)
{
    int fd = std::any_cast<int>(arg);

    /* Remove one fd, pid pair from our map */
    auto pid_result = m_pids.find(fd);
    if (pid_result == m_pids.end()) {
        ICP_LOG(ICP_LOG_ERROR, "Could not find pid for fd = %d\n", fd);
        return;
    }

    /* Check and see if this was the last fd for this pid. */
    if (m_pids.count(fd) == 1) {
        /* All other connections for this handler are gone; delete it */
        auto pid = pid_result->second;
        ICP_LOG(ICP_LOG_INFO, "All connections from pid %d are gone; cleaning up\n", pid);
        m_handlers.erase(pid);
    }

    m_pids.erase(pid_result);
    close(fd);
}

}

extern "C" {

const char* api_server_options_prefix_option_get(void)
{
    static std::string prefix_opt = icp::socket::api::prefix_option();
    return (prefix_opt.c_str());
}

}
