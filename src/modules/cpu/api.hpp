#ifndef _OP_CPU_API_HPP_
#define _OP_CPU_API_HPP_

#include <cinttypes>
#include <memory>
#include <string>
#include <variant>

#include <tl/expected.hpp>
#include <zmq.h>

#include "modules/dynamic/api.hpp"
#include "modules/timesync/chrono.hpp"

#include "common.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::message {
struct serialized_message;
}

namespace openperf::cpu::api {

using serialized_msg = openperf::message::serialized_message;

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
using id_ptr = std::unique_ptr<std::string>;

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
    std::vector<id_ptr> ids;
};

struct request_cpu_generator_start
{
    struct start_data
    {
        std::string id;
        dynamic::configuration dynamic_results;
    };

    std::unique_ptr<start_data> data;
};

struct request_cpu_generator_stop
{
    std::string id;
};

struct request_cpu_generator_bulk_start
{
    struct start_data
    {
        std::vector<std::string> ids;
        dynamic::configuration dynamic_results;
    };

    std::unique_ptr<start_data> data;
};

struct request_cpu_generator_bulk_stop
{
    std::vector<id_ptr> ids;
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

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

std::string to_string(const api::typed_error& error);
reply_error
to_error(error_type type, int code = 0, const std::string& value = "");

} // namespace openperf::cpu::api

#endif /* _OP_CPU_API_HPP_ */
