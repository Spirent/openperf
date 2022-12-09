#include "zmq.h"

#include "api/api_rest_error.hpp"

namespace openperf::api::rest {

std::pair<Pistache::Http::Code, std::optional<std::string>>
decode_error(const typed_error& error)
{
    switch (error.type) {
    case error_type::NOT_FOUND:
        return {Pistache::Http::Code::Not_Found, std::nullopt};
    case error_type::POSIX:
        return {Pistache::Http::Code::Bad_Request, strerror(error.value)};
    case error_type::ZMQ_ERROR:
        return {Pistache::Http::Code::Internal_Server_Error,
                zmq_strerror(error.value)};
    default:
        return {Pistache::Http::Code::Internal_Server_Error, "unknown error"};
    }
}

nlohmann::json to_json(int code, std::string_view message)
{
    return (nlohmann::json({{"code", code}, {"message", message.data()}}));
}

} // namespace openperf::api::rest
