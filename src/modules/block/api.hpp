#ifndef _OP_BLOCK_API_HPP_
#define _OP_BLOCK_API_HPP_

#include <string>

#include "json.hpp"

namespace openperf::block::api {

const std::string endpoint = "inproc://openperf_block";

/**
 * List of supported server requests.
 */
enum class request_type {
    NONE = 0,
    LIST_DEVICES,
    GET_DEVICE,
    LIST_FILES,
    CREATE_FILE,
    GET_FILE,
    DELETE_FILE,
    LIST_GENERATORS,
    CREATE_GENERATOR,
    GET_GENERATOR,
    DELETE_GENERATOR,
    START_GENERATOR,
    STOP_GENERATOR,
    BULK_START_GENERATORS,
    BULK_STOP_GENERATORS
};

enum class reply_code { NONE = 0, OK, NO_DEVICE, NO_FILE, NO_GENERATOR, BAD_INPUT, ERROR };

std::string to_string(request_type type);
std::string to_string(reply_code code);

inline std::string json_error(int code, const char* message)
{
    return (nlohmann::json({{"code", code}, {"message", message}}).dump());
}

extern const std::string endpoint;

} // namespace openperf::block::api
#endif /* _OP_BLOCK_API_HPP_ */
