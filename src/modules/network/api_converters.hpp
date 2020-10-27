#ifndef _OP_NETWORK_API_CONVERTERS_HPP_
#define _OP_NETWORK_API_CONVERTERS_HPP_

#include <json.hpp>
#include <tl/expected.hpp>

#include "api.hpp"

#include "swagger/v1/model/BulkCreateNetworkServersRequest.h"
#include "swagger/v1/model/BulkDeleteNetworkServersRequest.h"
#include "swagger/v1/model/BulkCreateNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopNetworkGeneratorsRequest.h"
#include "swagger/v1/model/NetworkGenerator.h"
#include "swagger/v1/model/NetworkGeneratorResult.h"
#include "swagger/v1/model/NetworkServer.h"

namespace openperf::network::api {

namespace swagger = ::swagger::v1::model;

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::NetworkGeneratorConfig&);
tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::NetworkServer&);

model::generator from_swagger(const swagger::NetworkGenerator&);
request::generator::bulk::create
from_swagger(swagger::BulkCreateNetworkGeneratorsRequest&);
request::generator::bulk::erase
from_swagger(swagger::BulkDeleteNetworkGeneratorsRequest&);
model::server from_swagger(const swagger::NetworkServer&);
request::server::bulk::create
from_swagger(swagger::BulkCreateNetworkServersRequest&);
request::server::bulk::erase
from_swagger(swagger::BulkDeleteNetworkServersRequest&);

std::shared_ptr<swagger::NetworkGenerator> to_swagger(const model::generator&);
std::shared_ptr<swagger::NetworkGeneratorResult>
to_swagger(const model::generator_result&);
std::shared_ptr<swagger::NetworkServer> to_swagger(const model::server&);

} // namespace openperf::network::api

#endif // _OP_NETWORK_API_CONVERTERS_HPP_
