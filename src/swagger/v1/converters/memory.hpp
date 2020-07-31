#ifndef _OP_MEMORY_JSON_CONVERTERS_HPP_
#define _OP_MEMORY_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class MemoryGenerator;
class BulkCreateMemoryGeneratorsRequest;
class BulkDeleteMemoryGeneratorsRequest;
class BulkStartMemoryGeneratorsRequest;
class BulkStopMemoryGeneratorsRequest;

void from_json(const nlohmann::json&, MemoryGenerator&);
void from_json(const nlohmann::json&, BulkCreateMemoryGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkDeleteMemoryGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStartMemoryGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStopMemoryGeneratorsRequest&);

} // namespace swagger::v1::model

#endif