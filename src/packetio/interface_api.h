#ifndef _ICP_PACKETIO_INTERFACE_API_H_
#define _ICP_PACKETIO_INTERFACE_API_H_

#include <string>

#include "json.hpp"

#include "core/icp_core.h"

namespace icp {

namespace core {
class event_loop;
}

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

const std::string& get_request_type(request_type type);
const std::string& get_reply_code(reply_code code);

inline std::string json_error(int code, const char *message)
{
    return (nlohmann::json({ {"code", code}, {"message", message} }).dump());
}

extern const std::string endpoint;

}
}
}
}
#endif /* _ICP_PACKETIO_INTERFACE_API_H_ */
