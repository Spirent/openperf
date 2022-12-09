#ifndef _OP_API_REST_ERRORS_HPP_
#define _OP_API_REST_ERRORS_HPP_

#include <utility>

#include "json.hpp"
#include "pistache/http.h"

namespace openperf::api::rest {

enum class error_type { NONE = 0, NOT_FOUND, ZMQ_ERROR, POSIX };

struct typed_error
{
    error_type type = error_type::NONE;
    int value = 0;
};

std::pair<Pistache::Http::Code, std::optional<std::string>>
decode_error(const typed_error&);

nlohmann::json to_json(int code, std::string_view message);

} // namespace openperf::api::rest

#endif /* _OP_API_REST_ERRORS_HPP_ */
