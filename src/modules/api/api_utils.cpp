
#include <string>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>

#include "api_service.hpp"
#include "api_internal_client.hpp"
#include "api/api_utils.hpp"

namespace openperf::api::utils {

static constexpr std::string_view api_server_host = "localhost";
static constexpr std::string_view api_check_resource = "/version";
static constexpr unsigned int poll_interval = 10; // in ms
static constexpr unsigned int max_poll_count = 6;

// Is the API port open?
static tl::expected<void, std::string> check_api_port()
{
    struct addrinfo hints = {.ai_family = AF_UNSPEC,
                             .ai_socktype = SOCK_STREAM};
    auto ai = Pistache::AddrInfo();
    int res =
        ai.invoke(api_server_host.data(),
                  std::to_string(openperf::api::api_get_service_port()).c_str(),
                  &hints);
    if (res != 0) {
        std::cerr << "Error starting up internal API client: "
                  << gai_strerror(res) << std::endl;
        return (tl::make_unexpected("Error starting up internal API client: "
                                    + std::string(gai_strerror(res))));
    }
    auto result = ai.get_info_ptr();
    unsigned int poll_count = 0;
    bool done = false;
    for (; (poll_count < max_poll_count) && !done; poll_count++) {
        // Experiments showed that a delay is always required.
        // Sleep first, check later.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(poll_interval * (1 << poll_count)));

        int sockfd =
            socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sockfd == -1) {
            return (
                tl::make_unexpected("Error starting up internal API client: "
                                    + std::string(strerror(errno))));
        }

        if (connect(sockfd, result->ai_addr, result->ai_addrlen) == 0) {
            done = true;
        }

        close(sockfd);
    }

    if (!done) {
        return (tl::make_unexpected("Error starting up internal API client. "
                                    "Could not connect to API server."));
    }

    return {};
}

// Is the API able to retrieve a resource?
tl::expected<void, std::string> check_api_path_ready(std::string_view path)
{
    unsigned int poll_count = 0;
    bool done = false;

    for (; (poll_count < max_poll_count) && !done; poll_count++) {
        auto [code, body] = openperf::api::client::internal_api_get(path);
        if (code == Pistache::Http::Code::Ok) {
            done = true;
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(poll_interval * (1 << poll_count)));
    }

    if (!done) {
        return (tl::make_unexpected("Error starting up internal API client. "
                                    "Could not retrieve resource: "
                                    + std::string(path)));
    }

    return {};
}

tl::expected<void, std::string> check_api_module_running()
{
    auto result = check_api_port();
    if (!result) return (result);

    result = check_api_path_ready(api_check_resource);
    if (!result) return (result);

    return {};
}

template <typename InputIterator, typename Function>
void for_each_chunked(
    InputIterator cursor,
    InputIterator last,
    typename std::iterator_traits<InputIterator>::difference_type chunk_size,
    Function func)
{
    while (cursor < last) {
        auto rem = std::distance(cursor, last);
        auto len = std::min(rem, chunk_size);
        func(cursor, len);
        cursor += len;
    }
}

void send_chunked_response(Pistache::Http::ResponseWriter response,
                           Pistache::Http::Code code,
                           const nlohmann::json& json)
{
    /* Note: the maximum response size is set to 1MB in api_init.cpp */
    constexpr auto chunk_size = 64 * 1024; /* 64 kB */

    auto tmp = json.dump();

    response.setMime(MIME(Application, Json));
    auto stream = response.stream(code);

    for_each_chunked(
        std::begin(tmp), std::end(tmp), chunk_size, [&](auto& it, size_t len) {
            /* Need address of the character from the iterator */
            stream.write(std::addressof(*it), len);
            stream.flush();
        });

    stream.ends();
}

} // namespace openperf::api::utils
