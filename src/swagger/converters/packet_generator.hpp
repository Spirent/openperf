#ifndef _OP_PACKET_GENERATOR_JSON_CONVERTERS_HPP_
#define _OP_PACKET_GENERATOR_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class PacketGenerator;
class PacketGeneratorConfig;
class PacketGeneratorResult;

void from_json(const nlohmann::json&, PacketGeneratorConfig&);
void from_json(const nlohmann::json&, PacketGenerator&);
void from_json(const nlohmann::json&, PacketGeneratorResult&);

} // namespace swagger::v1::model

#endif