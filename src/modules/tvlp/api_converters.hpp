#ifndef _OP_TVLP_API_CONVERTERS_HPP_
#define _OP_TVLP_API_CONVERTERS_HPP_

#include <iomanip>

#include "models/tvlp_config.hpp"
#include "models/tvlp_result.hpp"
#include "swagger/v1/model/TvlpConfiguration.h"
#include "swagger/v1/model/TvlpResult.h"

namespace openperf::tvlp::api {

namespace swagger = ::swagger::v1::model;
using namespace tvlp::model;

std::optional<time_point> from_rfc3339(const std::string& from);
tvlp_configuration_t from_swagger(const swagger::TvlpConfiguration&);
swagger::TvlpConfiguration to_swagger(const tvlp_configuration_t&);
swagger::TvlpResult to_swagger(const tvlp_result_t&);

} // namespace openperf::tvlp::api

#endif // _OP_TVLP_API_CONVERTERS_HPP_
