
#include <pistache/client.h>

#include "api_internal_client.hpp"
#include "api_service.hpp"

namespace openperf::api::client {

using namespace Pistache;

static auto internal_api_request(Http::RequestBuilder& request_builder,
                                 const std::string& body,
                                 duration_t timeout)
{
    // clang-format off
    auto response = request_builder
        .body(body)
        .send();
    // clang-format on

    std::pair<Http::Code, std::string> result;
    response.then(
        [&result](const Http::Response& response) {
            result.first = response.code();
            result.second = response.body();
        },
        Async::IgnoreException);

    Async::Barrier<Http::Response> barrier(response);
    barrier.wait_for(timeout);

    return result;
}

static auto make_full_uri(std::string_view resource)
{
    std::ostringstream os;
    auto transport_addr = api_get_service_transport_address();
    if (transport_addr.find(':') != std::string::npos) {
        os << "[" << transport_addr << "]"; // IPv6 needs to be bracketed
    } else {
        os << transport_addr;
    }
    os << ":" << api_get_service_port() << resource;
    return os.str();
}

std::pair<Http::Code, std::string> internal_api_get(std::string_view resource,
                                                    duration_t timeout)
{
    Http::Client client;
    client.init();
    auto rb = client.get(make_full_uri(resource));
    auto result = internal_api_request(rb, "", timeout);

    client.shutdown();

    return result;
}

std::pair<Http::Code, std::string> internal_api_post(std::string_view resource,
                                                     const std::string& body,
                                                     duration_t timeout)
{
    Http::Client client;
    client.init();
    auto rb = client.post(make_full_uri(resource));
    auto result = internal_api_request(rb, body, timeout);

    client.shutdown();

    return result;
}

std::pair<Http::Code, std::string> internal_api_del(std::string_view resource,
                                                    duration_t timeout)
{
    Http::Client client;
    client.init();
    auto rb = client.del(make_full_uri(resource));
    auto result = internal_api_request(rb, "", timeout);

    client.shutdown();

    return result;
}

} // namespace openperf::api::client
