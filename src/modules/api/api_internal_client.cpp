
#include <pistache/client.h>

#include "api_internal_client.hpp"
#include "api_service.hpp"

namespace openperf::api::client {

using namespace Pistache;

static const std::string server = "localhost";
static const int request_timeout = 1;

static auto internal_api_request(Http::RequestBuilder& request_builder,
                                 const std::string& body)
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
    barrier.wait_for(std::chrono::seconds(request_timeout));

    return result;
}

static auto make_full_uri(std::string_view resource)
{
    return server + ":" + std::to_string(api_get_service_port())
           + std::string(resource);
}

std::pair<Http::Code, std::string> internal_api_get(std::string_view resource)
{
    Http::Client client;
    client.init();
    auto rb = client.get(make_full_uri(resource));
    auto result = internal_api_request(rb, "");

    client.shutdown();

    return result;
}

std::pair<Http::Code, std::string> internal_api_post(std::string_view resource,
                                                     const std::string& body)
{
    Http::Client client;
    client.init();
    auto rb = client.post(make_full_uri(resource));
    auto result = internal_api_request(rb, body);

    client.shutdown();

    return result;
}

} // namespace openperf::api::client
