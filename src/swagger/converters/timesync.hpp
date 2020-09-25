#ifndef _OP_TIMESYNC_JSON_CONVERTERS_HPP_
#define _OP_TIMESYNC_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class TimeSource;

void from_json(const nlohmann::json&, TimeSource&);

} // namespace swagger::v1::model

#endif