#ifndef _OP_MEMORY_API_HPP_
#define _OP_MEMORY_API_HPP_

#include <string>

#include <zmq.h>

namespace openperf::memory::api {

constexpr std::string_view endpoint = "inproc://openperf_memory";

enum class request_type {
    NONE = 0,
    LIST_GENERATORS,
    CREATE_GENERATOR,
    GET_GENERATOR,
    DELETE_GENERATOR,
    START_GENERATOR,
    STOP_GENERATOR,
    BULK_START_GENERATORS,
    BULK_STOP_GENERATORS,
    LIST_RESULTS,
    GET_RESULT,
    DELETE_RESULT,
    GET_INFO
};

enum class reply_code {
    NONE = 0,
    OK,
    NO_GENERATOR,
    NO_RESULT,
    BAD_INPUT,
    ERROR
};

std::string to_string(request_type type);
std::string to_string(reply_code code);

} // namespace: openperf::memory::api

#endif // _OP_MEMORY_API_HPP_
