#ifndef _OP_NETWORK_JSON_CONVERTERS_HPP_
#define _OP_NETWORK_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class NetworkGenerator;
class BulkCreateNetworkGeneratorsRequest;
class BulkDeleteNetworkGeneratorsRequest;
class BulkStartNetworkGeneratorsRequest;
class BulkStopNetworkGeneratorsRequest;
class NetworkGeneratorResult;

void from_json(const nlohmann::json&, NetworkGenerator&);
void from_json(const nlohmann::json&, BulkCreateNetworkGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkDeleteNetworkGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStartNetworkGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStopNetworkGeneratorsRequest&);
void from_json(const nlohmann::json&, NetworkGeneratorResult&);

} // namespace swagger::v1::model

#endif