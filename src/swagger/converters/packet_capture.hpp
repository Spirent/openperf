#ifndef _OP_PACKET_CAPTURE_JSON_CONVERTERS_HPP_
#define _OP_PACKET_CAPTURE_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class PacketCapture;

void from_json(const nlohmann::json&, PacketCapture&);

} // namespace swagger::v1::model

#endif