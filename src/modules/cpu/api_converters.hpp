#ifndef _OP_CPU_API_CONVERTERS_HPP_
#define _OP_CPU_API_CONVERTERS_HPP_

#include <json.hpp>
#include <tl/expected.hpp>

#include "api.hpp"

#include "swagger/v1/model/BulkCreateCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopCpuGeneratorsRequest.h"
#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuGeneratorResult.h"
#include "swagger/v1/model/CpuInfoResult.h"

namespace openperf::cpu::api {

namespace swagger = ::swagger::v1::model;

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::CpuGeneratorConfig&);

model::generator from_swagger(const swagger::CpuGenerator&);
request_cpu_generator_bulk_add
from_swagger(swagger::BulkCreateCpuGeneratorsRequest&);
std::shared_ptr<swagger::CpuGenerator> to_swagger(const model::generator&);
std::shared_ptr<swagger::CpuGeneratorResult>
to_swagger(const model::generator_result&);
std::shared_ptr<swagger::CpuInfoResult> to_swagger(const cpu_info_t&);
} // namespace openperf::cpu::api

#endif // _OP_CPU_API_CONVERTERS_HPP_
