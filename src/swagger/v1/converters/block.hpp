#ifndef _OP_BLOCK_JSON_CONVERTERS_HPP_
#define _OP_BLOCK_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class BlockGenerator;
class BulkCreateBlockFilesRequest;
class BulkDeleteBlockFilesRequest;
class BulkCreateBlockGeneratorsRequest;
class BulkDeleteBlockGeneratorsRequest;
class BulkStartBlockGeneratorsRequest;
class BulkStopBlockGeneratorsRequest;
class BlockGeneratorResult;
class BlockFile;

void from_json(const nlohmann::json&, BlockFile&);
void from_json(const nlohmann::json&, BulkCreateBlockFilesRequest&);
void from_json(const nlohmann::json&, BulkDeleteBlockFilesRequest&);
void from_json(const nlohmann::json&, BlockGenerator&);
void from_json(const nlohmann::json&, BulkCreateBlockGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkDeleteBlockGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStartBlockGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStopBlockGeneratorsRequest&);
void from_json(const nlohmann::json&, BlockGeneratorResult&);

} // namespace swagger::v1::model

#endif