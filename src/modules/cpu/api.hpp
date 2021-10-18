#ifndef _OP_CPU_API_HPP_
#define _OP_CPU_API_HPP_

#include <cinttypes>
#include <memory>
#include <string>
#include <variant>

#include "tl/expected.hpp"
#include "zmq.h"

#include "modules/dynamic/api.hpp"

namespace swagger::v1::model {

class CpuGenerator;
class CpuGeneratorConfig;
class CpuGeneratorResult;
class CpuInfoResult;
class DynamicResultsConfig;

} // namespace swagger::v1::model

namespace openperf::message {
struct serialized_message;
}

namespace openperf::cpu::generator {
class coordinator;
class result;
struct config;
} // namespace openperf::cpu::generator

namespace openperf::cpu::api {

using serialized_msg = openperf::message::serialized_message;

const std::string endpoint = "inproc://openperf_cpu";

using cpu_generator_type = swagger::v1::model::CpuGenerator;
using cpu_generator_ptr = std::unique_ptr<cpu_generator_type>;

using cpu_generator_result_type = swagger::v1::model::CpuGeneratorResult;
using cpu_generator_result_ptr = std::unique_ptr<cpu_generator_result_type>;

using cpu_info_type = swagger::v1::model::CpuInfoResult;
using cpu_info_ptr = std::unique_ptr<cpu_info_type>;

enum class method_type { none = 0, system, cores };
method_type to_method_type(std::string_view name);
std::string to_string(method_type type);

enum class instruction_type {
    none = 0,
    scalar,
    sse2,
    sse4,
    avx,
    avx2,
    avx512,
    neon
};
instruction_type to_instruction_type(std::string_view name);
std::string to_string(instruction_type type);

enum class data_type { none = 0, int32, int64, float32, float64 };
data_type to_data_type(std::string_view name);
std::string to_string(data_type type);

/**
 * CPU server requests
 */

struct request_cpu_generator_list
{};

struct request_cpu_generator
{
    std::string id;
};

struct request_cpu_generator_add
{
    cpu_generator_ptr source;
};

struct request_cpu_generator_del
{
    std::string id;
};

struct request_cpu_generator_bulk_add
{
    std::vector<cpu_generator_ptr> generators;
};

struct request_cpu_generator_bulk_del
{
    std::vector<std::string> ids;
};

struct request_cpu_generator_start
{
    std::string id;
    dynamic::configuration dynamic_results;
};

struct request_cpu_generator_stop
{
    std::string id;
};

struct request_cpu_generator_bulk_start
{
    std::vector<std::string> ids;
    dynamic::configuration dynamic_results;
};

struct request_cpu_generator_bulk_stop
{
    std::vector<std::string> ids;
};

struct request_cpu_generator_result_list
{};

struct request_cpu_generator_result
{
    std::string id;
};

struct request_cpu_generator_result_del
{
    std::string id;
};

/*
 * CPU server replies
 */

struct reply_cpu_generators
{
    std::vector<cpu_generator_ptr> generators;
};

struct reply_cpu_generator_results
{
    std::vector<cpu_generator_result_ptr> results;
};

struct reply_ok
{};

enum class error_type { NONE = 0, NOT_FOUND, ZMQ_ERROR, POSIX };

struct typed_error
{
    error_type type = error_type::NONE;
    int value = 0;
};

struct reply_error
{
    typed_error info;
};

using request_msg = std::variant<request_cpu_generator_list,
                                 request_cpu_generator,
                                 request_cpu_generator_add,
                                 request_cpu_generator_del,
                                 request_cpu_generator_bulk_add,
                                 request_cpu_generator_bulk_del,
                                 request_cpu_generator_start,
                                 request_cpu_generator_stop,
                                 request_cpu_generator_bulk_start,
                                 request_cpu_generator_bulk_stop,
                                 request_cpu_generator_result_list,
                                 request_cpu_generator_result,
                                 request_cpu_generator_result_del>;

using reply_msg = std::variant<reply_cpu_generators,
                               reply_cpu_generator_results,
                               reply_ok,
                               reply_error>;

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

reply_error to_error(error_type type, int value = 0);
const char* to_string(const reply_error&);

cpu_generator_ptr to_swagger(std::string_view id,
                             const generator::coordinator& gen);
cpu_generator_result_ptr
to_swagger(std::string_view generator_id,
           std::string_view result_id,
           const std::shared_ptr<generator::result>& result);

cpu_info_ptr get_cpu_info();

generator::config from_swagger(const swagger::v1::model::CpuGeneratorConfig&);

bool is_valid(const cpu_generator_type&, std::vector<std::string>&);

bool is_valid(const swagger::v1::model::DynamicResultsConfig&,
              std::vector<std::string>&);

} // namespace openperf::cpu::api

#endif /* _OP_CPU_API_HPP_ */
