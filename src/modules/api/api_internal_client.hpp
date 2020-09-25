#ifndef _OP_API_INTERNAL_CLIENT_HPP_
#define _OP_API_INTERNAL_CLIENT_HPP_

#include <pistache/http.h>

namespace openperf::api::client {

using namespace std::chrono_literals;
using duration_t = std::chrono::seconds;

// Send a GET request to the REST API.
// @param resource - REST API resource perform GET operation on.
// @returns Pair of Http reponse code and response body.
std::pair<Pistache::Http::Code, std::string>
internal_api_get(std::string_view resource, duration_t timeout = 1s);

// Sent a POST request to the REST API.
// @param resource - REST API resource to perform POST operation on.
// @param body - Request body to include in the POST request.
// @returns Pair of Http reponse code and response body.
std::pair<Pistache::Http::Code, std::string>
internal_api_post(std::string_view resource,
                  const std::string& body,
                  duration_t timeout = 1s);

// Send a DELETE request to the REST API.
// @param resource - REST API resource perform DELETE operation on.
// @returns Pair of Http reponse code and response body.
std::pair<Pistache::Http::Code, std::string>
internal_api_del(std::string_view resource, duration_t timeout = 1s);

} // namespace openperf::api::client
#endif
