#ifndef _OP_PACKETIO_JSON_CONVERTERS_HPP_
#define _OP_PACKETIO_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace openperf::packetio {
static const std::string empty_id_string = "none";
} // namespace openperf::packetio

namespace swagger::v1::model {

class Interface;
class Port;

void from_json(const nlohmann::json&, Port&);

} // namespace swagger::v1::model

#endif
