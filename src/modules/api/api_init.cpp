#include <cerrno>
#include <cstdlib>
#include <csignal>
#include <thread>

#include <netinet/in.h>

#include "pistache/endpoint.h"
#include "pistache/router.h"

#include "core/icp_core.h"
#include "core/icp_log.h"
#include "config/icp_config_file.h"
#include "api/api_config_file_resources.h"
#include "api/api_route_handler.h"

namespace icp {
namespace api {

static in_port_t service_port = 8080;

static int module_version = 1;

using namespace Pistache;

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

in_port_t api_get_service_port(void)
{
    return service_port;
}

static
Rest::Route::Result NotFound(const Rest::Request &request __attribute__((unused)),
                             Http::ResponseWriter response)
{
    response.send(Http::Code::Not_Found);
    return Rest::Route::Result::Ok;
}

int api_configure_self()
{
    auto result = icp::config::file::icp_config_get_params("modules.api");
    if (!result) {
        ICP_LOG(ICP_LOG_ERROR, "%s", result.error().c_str());
        return (EINVAL);
    }

    YAML::Node root_node = result.value();

    if (root_node.size() == 0) {
        return (0);
    }

    if (!root_node.IsMap()) {
        ICP_LOG(ICP_LOG_ERROR, "api configuration node does not contain any values!");
        return (EINVAL);
    }

    if (root_node["port"]) {
        try {
            int res = set_service_port(root_node["port"].as<long>());
            if (res != 0) { return (res); }
        }
        catch (YAML::BadConversion &err) {
            ICP_LOG(ICP_LOG_ERROR, "Error converting configuration file value: %s", err.what());
            return (EINVAL);
        }
    }

    return (0);
}

class service {
public:
    service() {};

    int pre_init() {
        // Return 404 for anything we don't explicitly handle
        Rest::Routes::NotFound(m_router, NotFound);

        return (api_configure_self());
    }

    int post_init(void *context) {
        route::handler::make_all(m_handlers, context, m_router);
        return (0);
    }

    int start() {
        /*
         * XXX: The pistache server doesn't always shutdown, so just
         * detach from the thread so we won't be blocked on it when we
         * shutdown ourselves.
         */
        auto thread = std::thread([this]() { run(service_port); });
        thread.detach();

        auto ret = config::icp_config_file_process_resources();
        if (!ret) {
            std::cerr << ret.error() << std::endl;
            return (-1);
        }

        return (0);
    }

    void run(in_port_t port) {
        icp_thread_setname("icp_api");

        Address addr(Ipv4::any(), Port(port));
        auto opts = Http::Endpoint::options()
            .threads(1)
            .flags(Tcp::Options::ReuseAddr);

        m_server = std::make_unique<Http::Endpoint>(addr);
        m_server->init(opts);
        m_server->setHandler(m_router.handler());
        m_server->serve();        // blocks
    }

    void stop() {
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

}
}

extern "C" {

int api_service_pre_init(void *context, void *state)
{
    (void)context;
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->pre_init());
}

int api_service_post_init(void *context, void *state)
{
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->post_init(context));
}

int api_service_start(void *state)
{
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    return (s->start());
}

void api_service_fini(void *state)
{
    icp::api::service *s = reinterpret_cast<icp::api::service *>(state);
    s->stop();
    delete s;
}

REGISTER_MODULE(api_service,
                INIT_MODULE_INFO(
                                 "api",
                                 "Module to handle API services",
                                 icp::api::module_version
                                 ),
                new icp::api::service(),
                api_service_pre_init,
                nullptr,
                api_service_post_init,
                api_service_start,
                api_service_fini)
}
