#ifndef _OP_TVLP_API_CONVERTERS_HPP_
#define _OP_TVLP_API_CONVERTERS_HPP_

#include <tl/expected.hpp>

#include "models/tvlp_config.hpp"
#include "models/tvlp_result.hpp"
#include "swagger/v1/model/TvlpConfiguration.h"
#include "swagger/v1/model/TvlpStartConfiguration.h"
#include "swagger/v1/model/TvlpResult.h"

namespace openperf::tvlp::api {

namespace swagger = ::swagger::v1::model;
using namespace tvlp::model;

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::TvlpStartConfiguration&);

std::optional<time_point> from_rfc3339(const std::string& from);
void apply_defaults(swagger::TvlpStartConfiguration&);

tvlp_start_t from_swagger(const swagger::TvlpStartConfiguration&);
tvlp_configuration_t from_swagger(const swagger::TvlpConfiguration&);
swagger::TvlpConfiguration to_swagger(const tvlp_configuration_t&);
swagger::TvlpResult to_swagger(const tvlp_result_t&);

} // namespace openperf::tvlp::api

#endif // _OP_TVLP_API_CONVERTERS_HPP_
