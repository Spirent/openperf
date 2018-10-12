#ifndef _ICP_PACKETIO_PORT_API_H_
#define _ICP_PACKETIO_PORT_API_H_

#include <memory>
#include <string>
#include <vector>

#include "json.hpp"

namespace swagger {
namespace v1 {
namespace model {
class Port;
}
}
}

namespace icp {

namespace core {
class event_loop;
}

namespace packetio {
namespace port {
namespace api {

enum class request_type { NONE = 0,
                          LIST_PORTS,
                          CREATE_PORT,
                          GET_PORT,
                          UPDATE_PORT,
                          DELETE_PORT };

/**
 * List of server reply codes.  Not all codes are returned by all requests.
 */
enum class reply_code { NONE = 0,
                        OK,           /**< Request completed without errors */
                        NO_PORT,      /**< Specified port is invalid */
                        BAD_INPUT,    /**< User supplied input could not be parsed */
                        ERROR };      /**< Internal error */

std::string to_string(request_type type);
std::string to_string(reply_code code);

inline std::string json_error(int code, const char *message)
{
    return (nlohmann::json({ {"code", code }, {"message", message} }).dump());
}

/**
 * Backend API functions
 */
namespace impl {

bool is_valid_port(int port_idx);
void list_ports(std::vector<std::shared_ptr<swagger::v1::model::Port>> &ports,
                std::string &kind);
std::shared_ptr<swagger::v1::model::Port> get_port(int port_idx);

void delete_port(int port_idx);
}

extern const std::string endpoint;

}
}
}
}

#endif /*_ICP_PACKETIO_PORT_API_H_ */
