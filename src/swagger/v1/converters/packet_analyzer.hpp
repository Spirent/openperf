#ifndef _OP_PACKET_ANALYZER_JSON_CONVERTERS_HPP_
#define _OP_PACKET_ANALYZER_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class PacketAnalyzer;

void from_json(const nlohmann::json&, PacketAnalyzer&);

} // namespace swagger::v1::model

#endif