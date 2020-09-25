#ifndef _OP_MEMORY_API_CONVERTERS_HPP_
#define _OP_MEMORY_API_CONVERTERS_HPP_

#include <json.hpp>

#include "api.hpp"
#include "pistache/http_defs.h"
#include "swagger/v1/model/BulkCreateMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopMemoryGeneratorsRequest.h"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorConfig.h"
#include "swagger/v1/model/MemoryGeneratorResult.h"
#include "swagger/v1/model/MemoryGeneratorStats.h"
#include "swagger/v1/model/MemoryInfoResult.h"

namespace openperf::memory::api {

namespace swagger = ::swagger::v1::model;
using namespace memory::internal;

config_t from_swagger(const swagger::MemoryGeneratorConfig&);
swagger::MemoryGeneratorConfig to_swagger(const generator::config_t&);
swagger::MemoryGenerator to_swagger(const reply::generator::item&);
swagger::MemoryGeneratorStats to_swagger(const task_memory_stat&);
swagger::MemoryGeneratorResult to_swagger(const reply::statistic::item&);
swagger::MemoryInfoResult to_swagger(const memory_info::info_t&);

std::pair<Pistache::Http::Code, std::optional<std::string>>
to_error(const reply::error& error);

} // namespace openperf::memory::api

#endif // _OP_MEMORY_API_CONVERTERS_HPP_
