#ifndef _OP_CPU_JSON_CONVERTERS_HPP_
#define _OP_CPU_JSON_CONVERTERS_HPP_

#include "json.hpp"

namespace swagger::v1::model {

class CpuGenerator;
class BulkCreateCpuGeneratorsRequest;
class BulkDeleteCpuGeneratorsRequest;
class BulkStartCpuGeneratorsRequest;
class BulkStopCpuGeneratorsRequest;

void from_json(const nlohmann::json&, CpuGenerator&);
void from_json(const nlohmann::json&, BulkCreateCpuGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkDeleteCpuGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStartCpuGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStopCpuGeneratorsRequest&);

} // namespace swagger::v1::model

#endif