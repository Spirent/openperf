#ifndef _OP_MEMORY_SWAGGER_CONVERTERS_HPP_
#define _OP_MEMORY_SWAGGER_CONVERTERS_HPP_

#include <iomanip>

#include "model/tvlp_config.hpp"
#include "swagger/v1/model/TvlpConfiguration.h"

namespace openperf::tvlp::api {

using config_t = tvlp::model::tvlp_configuration_t;

namespace swagger = ::swagger::v1::model;

static config_t from_swagger(const swagger::TvlpConfiguration& m)
{
    return config_t{};
}

static swagger::TvlpConfiguration to_swagger(const config_t& config)
{
    swagger::TvlpConfiguration model;

    return model;
}

} // namespace openperf::tvlp::api

#endif // _OP_MEMORY_SWAGGER_CONVERTERS_HPP_
