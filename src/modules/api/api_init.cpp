#include <cerrno>
#include <cstdlib>
#include <csignal>
#include <thread>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "pistache/endpoint.h"
#include "pistache/router.h"

#include "core/op_core.h"
#include "core/op_log.h"
#include "config/op_config_file.hpp"
#include "api/api_config_file_resources.hpp"
#include "api/api_route_handler.hpp"

namespace openperf::api {

using namespace Pistache;

static constexpr std::string_view localhost = "localhost";

static in_port_t service_port = 8080;

static IP service_address = IP::any(Ipv6::supported());

static std::string service_transport_address(localhost);

static int module_version = 1;

static int set_service_port(long port)
{
    if (port == 0) {
        return (EINVAL);
    } else if (port > 65535) {
        return (ERANGE);
    } else {
        service_port = static_cast<in_port_t>(port);
    }

    return (0);
}

in_port_t api_get_service_port(void) { return service_port; }

static int set_service_address(const std::string& ip_str)
{
    uint8_t data[sizeof(in6_addr)];

    if (inet_pton(AF_INET6, ip_str.c_str(), data)) {
        struct sockaddr_in6 saddr = {};
        saddr.sin6_family = AF_INET6;
        memcpy(
            &(saddr.sin6_addr.s6_addr), data, sizeof(saddr.sin6_addr.s6_addr));
        service_address = IP(reinterpret_cast<struct sockaddr*>(&saddr));
        if (IN6_IS_ADDR_UNSPECIFIED(&saddr.sin6_addr)) {
            // Use localhost when bound to any IPv6 address
            service_transport_address = localhost;
        } else {
            service_transport_address = service_address.toString();
        }
        return (0);
    }

    if (inet_pton(AF_INET, ip_str.c_str(), data)) {
        struct sockaddr_in saddr = {};
        saddr.sin_family = AF_INET;
        memcpy(&(saddr.sin_addr.s_addr), data, sizeof(saddr.sin_addr.s_addr));
        service_address = IP(reinterpret_cast<struct sockaddr*>(&saddr));
        if (saddr.sin_addr.s_addr == INADDR_ANY) {
            // Use localhost when bound to any IPv4 address
            service_transport_address = localhost;
        } else {
            service_transport_address = service_address.toString();
        }
        return (0);
    }

    // Lookup named addresses e.g. "localhost", "ip6-localhost"

    struct addrinfo hints = {.ai_family = AF_UNSPEC,
                             .ai_socktype = SOCK_STREAM};
    AddrInfo ai;
    if (auto err = ai.invoke(
            ip_str.c_str(), std::to_string(service_port).c_str(), &hints)) {
        return err;
    }

    for (const addrinfo* addr = ai.get_info_ptr(); addr; addr = addr->ai_next) {
        if (addr->ai_family == AF_INET || addr->ai_family == AF_INET6) {
            service_address = IP(addr->ai_addr);
            service_transport_address = service_address.toString();
            return (0);
        }
    }

    return (EINVAL);
}

const IP& api_get_service_address(void) { return service_address; }

std::string api_get_service_transport_address(void)
{
    return service_transport_address;
}

static void block_sigpipe()
{
    /* Add SIGPIPE to the list of signals blocked for this thread */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    if (auto error = pthread_sigmask(SIG_BLOCK, &set, nullptr)) {
        throw std::runtime_error("Could not block SIGPIPE: "
                                 + std::string(strerror(error)));
    }
}

static Rest::Route::Result NotFound(const Rest::Request& request
                                    __attribute__((unused)),
                                    Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Found);
    return Rest::Route::Result::Ok;
}

int api_configure_self()
{
    auto port_num =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_LONG>(
            "modules.api.port");

    if (port_num) {
        if (auto err = set_service_port(*port_num)) { return err; }
    }

    auto ip_addr =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_STRING>(
            "modules.api.address");

    if (ip_addr) {
        if (auto err = set_service_address(*ip_addr)) { return err; }
    }

    return (0);
}

class service
{
public:
    service() = default;
    ;

    int pre_init()
    {
        // Return 404 for anything we don't explicitly handle
        Rest::Routes::NotFound(m_router, NotFound);

        return (api_configure_self());
    }

    int post_init(void* context)
    {
        route::handler::make_all(m_handlers, context, m_router);
        return (0);
    }

    int start()
    {
        /*
         * XXX: The pistache server doesn't always shutdown, so just
         * detach from the thread so we won't be blocked on it when we
         * shutdown ourselves.
         */
        auto service_thread =
            std::thread([this]() { run(service_address, service_port); });
        OP_LOG(OP_LOG_DEBUG,
               "REST API server listening on %s port %d",
               service_address.toString().c_str(),
               service_port);
        service_thread.detach();

        /*
         * Spawn a thread to populate configuration; modules might load after us
         * and we don't want to block them.
         */
        auto config_thread = std::thread([]() {
            op_thread_setname("op_api_config");
            auto ret = config::op_config_file_process_resources();
            if (!ret) {
                throw std::runtime_error(
                    "Configuration file processing failed: " + ret.error());
            }
        });
        config_thread.detach();

        return (0);
    }

    void run(IP ip_addr, in_port_t port)
    {
        op_thread_setname("op_api");

        /*
         * Pistache doesn't handle SIGPIPE, so block it for this thread and all
         * its descendants
         */
        block_sigpipe();

        Address addr(ip_addr, Port(port));
        auto opts = Http::Endpoint::options()
                        .maxRequestSize(1024 * 1024)
                        .maxResponseSize(1024 * 1024)
                        .threads(1)
                        .flags(Tcp::Options::ReuseAddr);

        m_server = std::make_unique<Http::Endpoint>(addr);
        m_server->init(opts);
        m_server->setHandler(m_router.handler());
        m_server->serve(); // blocks
    }

    void stop()
    {
        if (m_server) {
            m_server->shutdown();
            m_server.reset();
        }
    }

private:
    Rest::Router m_router;
    std::vector<std::unique_ptr<route::handler>> m_handlers;
    std::unique_ptr<Http::Endpoint> m_server;
};

} // namespace openperf::api

extern "C" {

int api_service_pre_init(void* context, void* state)
{
    (void)context;
    auto* s = reinterpret_cast<openperf::api::service*>(state);
    return (s->pre_init());
}

int api_service_post_init(void* context, void* state)
{
    auto* s = reinterpret_cast<openperf::api::service*>(state);
    return (s->post_init(context));
}

int api_service_start(void* state)
{
    auto* s = reinterpret_cast<openperf::api::service*>(state);
    return (s->start());
}

void api_service_fini(void* state)
{
    auto* s = reinterpret_cast<openperf::api::service*>(state);
    s->stop();
    delete s;
}

REGISTER_MODULE(api_service,
                INIT_MODULE_INFO("api",
                                 "Module to handle API services",
                                 openperf::api::module_version),
                new openperf::api::service(),
                api_service_pre_init,
                nullptr,
                api_service_post_init,
                api_service_start,
                api_service_fini)
}
