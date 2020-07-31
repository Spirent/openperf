#ifndef _OP_PACKET_GENERATOR_JSON_CONVERTERS_HPP_
#define _OP_PACKET_GENERATOR_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class PacketGenerator;
class PacketGeneratorConfig;

void from_json(const nlohmann::json&, PacketGeneratorConfig&);
void from_json(const nlohmann::json&, PacketGenerator&);

} // namespace swagger::v1::model

#endif