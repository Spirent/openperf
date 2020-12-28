#ifndef _OP_BLOCK_API_CONVERTERS_HPP_
#define _OP_BLOCK_API_CONVERTERS_HPP_

#include <json.hpp>

#include "api.hpp"

#include "swagger/v1/model/BlockDevice.h"
#include "swagger/v1/model/BlockFile.h"
#include "swagger/v1/model/BlockGenerator.h"
#include "swagger/v1/model/BlockGeneratorResult.h"
#include "swagger/v1/model/BulkCreateBlockFilesRequest.h"
#include "swagger/v1/model/BulkCreateBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteBlockFilesRequest.h"
#include "swagger/v1/model/BulkDeleteBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartBlockGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopBlockGeneratorsRequest.h"

namespace openperf::block::api {

using namespace swagger::v1::model;

bool is_valid(const BlockFile& generator, std::vector<std::string>& errors);
bool is_valid(const BlockGenerator& generator,
              std::vector<std::string>& errors);

std::shared_ptr<BlockDevice> to_swagger(const model::device&);
std::shared_ptr<BlockFile> to_swagger(const model::file&);
std::shared_ptr<BlockGenerator> to_swagger(const model::block_generator&);
std::shared_ptr<BlockGeneratorResult>
to_swagger(const model::block_generator_result&);

model::file from_swagger(const BlockFile&);
model::block_generator from_swagger(const BlockGenerator&);
request_block_file_bulk_add from_swagger(BulkCreateBlockFilesRequest&);
request_block_generator_bulk_add
from_swagger(BulkCreateBlockGeneratorsRequest&);
request_block_generator_bulk_start
from_swagger(BulkStartBlockGeneratorsRequest&);

} // namespace openperf::block::api

#endif // _OP_BLOCK_API_CONVERTERS_HPP_
