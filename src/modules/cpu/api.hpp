#ifndef _OP_CPU_API_HPP_
#define _OP_CPU_API_HPP_

#include <cinttypes>
#include <memory>
#include <string>
#include <variant>

#include <tl/expected.hpp>
#include <json.hpp>
#include <zmq.h>

#include "modules/timesync/chrono.hpp"

#include "common.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace swagger::v1::model {
class CpuGenerator;
class CpuGeneratorResult;
class BulkCreateCpuGeneratorsRequest;
class BulkDeleteCpuGeneratorsRequest;
class BulkStartCpuGeneratorsRequest;
class BulkStopCpuGeneratorsRequest;
class CpuInfoResult;

void from_json(const nlohmann::json&, CpuGenerator&);
void from_json(const nlohmann::json&, BulkCreateCpuGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkDeleteCpuGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStartCpuGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStopCpuGeneratorsRequest&);
} // namespace swagger::v1::model

namespace openperf::cpu::api {

using namespace swagger::v1::model;

const std::string endpoint = "inproc://openperf_cpu";

/**
 * List of supported server requests.
 */

/* zmq api objects models */

struct cpu_info_t
{
    uint16_t cores;
    uint16_t cache_line_size;
    std::string architecture;
};

using cpu_generator_t = model::generator;
using cpu_generator_result_t = model::generator_result;
using cpu_generator_ptr = std::unique_ptr<cpu_generator_t>;
using cpu_generator_result_ptr = std::unique_ptr<cpu_generator_result_t>;
using cpu_info_ptr = std::unique_ptr<cpu_info_t>;

enum class error_type { NONE = 0, NOT_FOUND, ZMQ_ERROR, CUSTOM_ERROR };

struct typed_error
{
    error_type type = error_type::NONE;
    int code = 0;
    std::string value;
};

/* zmq api request models */

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
    std::unique_ptr<std::vector<std::string>> ids;
};

struct request_cpu_generator_start
{
    std::string id;
};

struct request_cpu_generator_stop
{
    std::string id;
};

struct request_cpu_generator_bulk_start
{
    std::unique_ptr<std::vector<std::string>> ids;
};

struct request_cpu_generator_bulk_stop
{
    std::unique_ptr<std::vector<std::string>> ids;
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

struct request_cpu_info
{};

/* zmq api reply models */

struct reply_cpu_generators
{
    std::vector<cpu_generator_ptr> generators;
};

struct reply_cpu_generator_results
{
    std::vector<cpu_generator_result_ptr> results;
};

struct reply_cpu_info
{
    cpu_info_ptr info;
};

struct reply_ok
{};

struct reply_error
{
    std::unique_ptr<typed_error> info;
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
                                 request_cpu_generator_result_del,
                                 request_cpu_info>;

using reply_msg = std::variant<reply_cpu_generators,
                               reply_cpu_generator_results,
                               reply_cpu_info,
                               reply_ok,
                               reply_error>;

struct serialized_msg
{
    zmq_msg_t type;
    zmq_msg_t data;
};

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg);
tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg);

int send_message(void* socket, serialized_msg&& msg);
tl::expected<serialized_msg, int> recv_message(void* socket, int flags = 0);

reply_error
to_error(error_type type, int code = 0, const std::string& value = "");
std::string to_string(const api::typed_error&);
model::generator from_swagger(const CpuGenerator&);
request_cpu_generator_bulk_add from_swagger(BulkCreateCpuGeneratorsRequest&);
request_cpu_generator_bulk_del from_swagger(BulkDeleteCpuGeneratorsRequest&);
std::shared_ptr<CpuGenerator> to_swagger(const model::generator&);
std::shared_ptr<CpuGeneratorResult> to_swagger(const model::generator_result&);
std::shared_ptr<CpuInfoResult> to_swagger(const cpu_info_t&);

extern const std::string endpoint;

} // namespace openperf::cpu::api

#endif /* _OP_CPU_API_HPP_ */
