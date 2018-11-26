#ifndef _ICP_PACKETIO_STACK_API_H_
#define _ICP_PACKETIO_STACK_API_H_

#include <string>

#include "json.hpp"

namespace icp {
namespace packetio {
namespace stack {
namespace api {

enum class request_type { NONE = 0,
                          LIST_STACKS,
                          GET_STACK };

enum class reply_code { NONE = 0,
                        OK,
                        NO_STACK,
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
#endif /* _ICP_PACKETIO_STACK_API_H_ */
