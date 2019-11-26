#ifndef _OP_PACKETIO_PORT_API_HPP_
#define _OP_PACKETIO_PORT_API_HPP_

#include <memory>
#include <string>
#include <vector>

#include "json.hpp"

namespace openperf {

namespace core {
class event_loop;
}

namespace packetio {
namespace port {
namespace api {

enum class request_type {
    NONE = 0,
    LIST_PORTS,
    CREATE_PORT,
    GET_PORT,
    UPDATE_PORT,
    DELETE_PORT
};

/**
 * List of server reply codes.  Not all codes are returned by all requests.
 */
enum class reply_code {
    NONE = 0,
    OK,        /**< Request completed without errors */
    NO_PORT,   /**< Specified port is invalid */
    BAD_INPUT, /**< User supplied input could not be parsed */
    ERROR
}; /**< Internal error */

std::string to_string(request_type type);
std::string to_string(reply_code code);

inline std::string json_error(int code, const char* message)
{
    return (nlohmann::json({{"code", code}, {"message", message}}).dump());
}

extern const std::string endpoint;

} // namespace api
} // namespace port
} // namespace packetio
} // namespace openperf

#endif /*_OP_PACKETIO_PORT_API_HPP_ */
