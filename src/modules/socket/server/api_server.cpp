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

#include "socket/api.hpp"
#include "socket/dgram_channel.hpp"
#include "socket/process_control.hpp"
#include "socket/stream_channel.hpp"
#include "socket/server/api_handler.hpp"
#include "socket/server/api_server.hpp"
#include "core/op_core.h"
#include "core/op_uuid.hpp"
#include "op_config_file.hpp"

namespace openperf::socket::api {

using api_handler = openperf::socket::server::api_handler;

static constexpr std::string_view shm_file_prefix = "/dev/shm/";

static __attribute__((const)) uint64_t align_up(uint64_t x, uint64_t align)
{
    return ((x + align - 1) & ~(align - 1));
}

bool unlink_stale_files()
{
    auto result = config::file::op_config_get_param<OP_OPTION_TYPE_NONE>(
        "modules.socket.force-unlink");

    return (result.value_or(false));
}

std::string prefix_option()
{
    auto result = config::file::op_config_get_param<OP_OPTION_TYPE_STRING>(
        "modules.socket.prefix");

    return (result.value_or(std::string()));
}

static openperf::memory::shared_segment create_shared_memory(size_t size)
{
    auto prefix_name = prefix_option();

    auto shared_segment_name =
        (prefix_name.length() > 0
             ? std::string(api::key) + ".memory." + prefix_name
             : std::string(api::key) + ".memory");
    if (access((std::string(shm_file_prefix) + shared_segment_name).c_str(),
               F_OK)
            != -1
        && unlink_stale_files()) {
        OP_LOG(OP_LOG_DEBUG,
               "Unlinking stale shared memory at: %s",
               shared_segment_name.c_str());
        if (shm_unlink(shared_segment_name.data()) < 0) {
            throw std::runtime_error("Could not remove shared memory segment "
                                     + std::string(shared_segment_name) + ": "
                                     + strerror(errno));
        }
    }

    auto impl_size = align_up(sizeof(socket::server::allocator), 64);
    auto segment = openperf::memory::shared_segment(
        shared_segment_name.data(), size, true);

    /* Construct our allocator in our shared memory segment */
    new (segment.base()) socket::server::allocator(
        reinterpret_cast<uintptr_t>(segment.base()) + impl_size,
        size - impl_size);

    OP_LOG(OP_LOG_DEBUG,
           "Created shared memory at: %s, with size %lu",
           shared_segment_name.c_str(),
           size);

    return (segment);
}

socket::server::allocator* server::allocator() const
{
    return (reinterpret_cast<socket::server::allocator*>(m_shm.base()));
}

static openperf::socket::unix_socket
create_unix_socket(const std::string_view path, int type)
{
    // Is there a prefix for the path?
    std::string full_path(path);
    if (auto prefix_name = prefix_option(); prefix_name.length() > 0) {
        full_path += "." + prefix_name;
    }

    if (access(full_path.c_str(), F_OK) != -1 && unlink_stale_files()) {
        OP_LOG(OP_LOG_DEBUG,
               "Unlinking stale server socket at: %s",
               full_path.c_str());
        if (unlink(full_path.c_str()) < 0) {
            throw std::runtime_error(
                "Could not remove shared unix domain socket " + full_path + ": "
                + strerror(errno));
        }
    }

    auto socket = openperf::socket::unix_socket(full_path, type);

    OP_LOG(OP_LOG_DEBUG, "Created unix socket at: %s", full_path.c_str());

    return (socket);
}

static ssize_t log_ptrace_error(void*, const char* buf, size_t size)
{
    if (size == 0) return (0);

    /*
     * Our op_log functions needs all logging messages terminated with a
     * new-line, so fix up any messages that need it.
     */
    const char* format = (buf[size - 1] == '\n') ? "%.*s" : "%.*s\n";

    /*
     * About that function tag...  The logging macro can easily retrieve
     * the current function name.  However, this function's name isn't helpful,
     * so skip the macro and just fill in the actual logging function's
     * arguments as appropriate for our usage.
     */
    op_log(OP_LOG_ERROR,
           "openperf::socket::process_control::enable_ptrace",
           format,
           static_cast<int>(size),
           buf);

    return (size);
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

    /*
     * The process control code is used by both server AND client.  As a
     * result, it doesn't use the default core logger, but allows callers
     * to redirect error messages to arbitrary streams.  Construct a
     * temporary "cookie" for the process control code to use.
     */
    auto stream_functions = cookie_io_functions_t{.write = log_ptrace_error};

    auto stream = std::unique_ptr<FILE, decltype(&fclose)>(
        fopencookie(nullptr, "w+", stream_functions), fclose);
    if (!stream) {
        throw std::runtime_error(
            "Could not create error stream for process control: "
            + std::string(strerror(errno)));
    }

    if (!process_control::enable_ptrace(stream.get())) {
        throw std::runtime_error("Could not enable process tracing");
    }
}

server::~server() {}

int server::start()
{
    auto client = packetio::internal::api::client(m_context);

    assert(m_task.empty());

    using namespace std::placeholders;
    auto result =
        client.add_task(packetio::workers::context::STACK,
                        "socket API",
                        m_sock.get(),
                        std::bind(&server::handle_api_accept, this, _1, _2),
                        nullptr);
    if (!result) { return (result.error()); }

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
        auto fd = accept(
            m_sock.get(), reinterpret_cast<sockaddr*>(&client), &client_length);
        if (fd == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                OP_LOG(OP_LOG_ERROR,
                       "Could not accept socket: %s\n",
                       strerror(errno));
            }
            break;
        }

        using namespace std::placeholders;
        if (!loop.add_callback(
                "socket API for fd = " + std::to_string(fd),
                fd,
                std::bind(&server::handle_api_client, this, _1, _2),
                std::bind(&server::handle_api_error, this, _1),
                fd)) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed to add socket API callback for fd = %d\n",
                   fd);
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
    api::request_msg request;
    auto ret = recv(fd, &request, sizeof(request), MSG_DONTWAIT);
    if (ret == -1) return (0);

    if (!std::holds_alternative<api::request_init>(request)) {
        OP_LOG(OP_LOG_ERROR,
               "Received unexpected message during "
               "client init phase");
        api::reply_msg reply = tl::make_unexpected(EINVAL);
        send(fd, &reply, sizeof(reply), 0);
        return (0);
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
        OP_LOG(OP_LOG_INFO,
               "New connection received from pid %d, %s\n",
               init.pid,
               to_string(init.tid).c_str());
        m_handlers.emplace(init.pid,
                           std::make_unique<api_handler>(
                               loop, m_shm.base(), *(allocator()), init.pid));
        auto shm_info = api::shared_memory_descriptor{.size = m_shm.size()};
        strncpy(
            shm_info.name, m_shm.name().data(), api::shared_memory_name_length);
        reply = api::reply_init{.pid = getpid(),
                                .shm_info = std::make_optional(shm_info)};
    } else {
        /*
         * We have already responded to an init request for this pid.
         * Since the client should already have the shared memory
         * data, don't return it again.
         */
        reply = api::reply_init{.pid = getpid(), .shm_info = std::nullopt};
    }

    if (send(fd, &reply, sizeof(reply), 0) > 0) {
        OP_LOG(OP_LOG_DEBUG,
               "Initialized client from pid %d, %s\n",
               init.pid,
               to_string(init.tid).c_str());

        /* Add this client to our pid map */
        m_pids.emplace(fd, init.pid);
    }

    /*
     * Jump straight into servicing client requests.  We need to clear
     * the socket of any messages before returning, and the client might
     * have sent one already.
     */
    return (do_client_read(init.pid, fd));
}

int server::do_client_read(pid_t pid, int fd)
{
    /* Find the handler for this pid and dispatch to it */
    auto handler_result = m_handlers.find(pid);
    if (handler_result == m_handlers.end()) {
        OP_LOG(OP_LOG_ERROR, "Could not locate handler for pid = %d\n", pid);
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
        OP_LOG(OP_LOG_ERROR, "Could not find pid for fd = %d\n", fd);
        return;
    }

    /* Check and see if this was the last fd for this pid. */
    if (m_pids.count(fd) == 1) {
        /* All other connections for this handler are gone; delete it */
        auto pid = pid_result->second;
        OP_LOG(OP_LOG_INFO,
               "All connections from pid %d are gone; cleaning up\n",
               pid);
        m_handlers.erase(pid);
    }

    m_pids.erase(pid_result);
    close(fd);
}

} // namespace openperf::socket::api

extern "C" {

const char* api_server_options_prefix_option_get(void)
{
    static std::string prefix_opt = openperf::socket::api::prefix_option();
    return (prefix_opt.c_str());
}
}
