#ifndef _OP_PACKETIO_INTERFACE_API_HPP_
#define _OP_PACKETIO_INTERFACE_API_HPP_

#include <string>

#include "json.hpp"

namespace openperf {
namespace packetio {
namespace interface {
namespace api {

/**
 * List of supported server requests.
 */
enum class request_type { NONE = 0,
                          LIST_INTERFACES,
                          CREATE_INTERFACE,
                          GET_INTERFACE,
                          DELETE_INTERFACE,
                          BULK_CREATE_INTERFACES,
                          BULK_DELETE_INTERFACES };

enum class reply_code { NONE = 0,
                        OK,
                        NO_INTERFACE,
                        BAD_INPUT,
                        ERROR };

std::string to_string(request_type type);
std::string to_string(reply_code code);

inline std::string json_error(int code, const char *message)
{
    return (nlohmann::json({ {"code", code}, {"message", message} }).dump());
}

extern const std::string endpoint;

}
}
}
}
#endif /* _OP_PACKETIO_INTERFACE_API_HPP_ */
