#ifndef _OP_CPU_API_HPP_
#define _OP_CPU_API_HPP_

#include <chrono>
#include <limits.h>
#include <string>
#include <variant>
#include <tl/expected.hpp>
#include "json.hpp"
#include "timesync/chrono.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"
#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuGeneratorResult.h"
#include "swagger/v1/model/BulkStartCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopCpuGeneratorsRequest.h"

namespace openperf::cpu::api {

using namespace swagger::v1::model;
using time_point = std::chrono::time_point<timesync::chrono::realtime>;

const std::string endpoint = "inproc://openperf_cpu";
static constexpr std::string_view empty_id_string = "none";

/**
 * List of supported server requests.
 */

static constexpr size_t id_max_length = 64;
static constexpr size_t err_max_length = 256;

/* zmq api objects models */

using cpu_generator_t = model::cpu_generator;
using cpu_generator_result_t = model::cpu_generator_result;
using cpu_generator_ptr = std::unique_ptr<cpu_generator_t>;
using cpu_generator_result_ptr = std::unique_ptr<cpu_generator_result_t>;
using string_ptr = std::unique_ptr<std::string>;

enum class error_type {
    NONE = 0,
    NOT_FOUND,
    EAI_ERROR,
    ZMQ_ERROR,
    CUSTOM_ERROR
};

struct typed_error
{
    error_type type = error_type::NONE;
    int code = 0;
    char value[err_max_length];
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
    std::vector<string_ptr> ids;
};

struct request_cpu_generator_bulk_stop
{
    std::vector<string_ptr> ids;
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

/* zmq api reply models */

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

struct reply_error
{
    typed_error info;
};

using request_msg = std::variant<request_cpu_generator_list,
                                 request_cpu_generator,
                                 request_cpu_generator_add,
                                 request_cpu_generator_del,
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

reply_error to_error(error_type type, int code = 0, std::string value = "");
const char* to_string(const api::typed_error&);
model::cpu_generator from_swagger(const CpuGenerator&);
request_cpu_generator_bulk_start from_swagger(BulkStartCpuGeneratorsRequest&);
request_cpu_generator_bulk_stop from_swagger(BulkStopCpuGeneratorsRequest&);
std::shared_ptr<CpuGenerator> to_swagger(const model::cpu_generator&);
std::shared_ptr<CpuGeneratorResult> to_swagger(const model::cpu_generator_result&);

extern const std::string endpoint;

} // namespace openperf::cpu::api

#endif /* _OP_CPU_API_HPP_ */