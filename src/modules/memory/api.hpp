#ifndef _OP_MEMORY_API_HPP_
#define _OP_MEMORY_API_HPP_

#include <string>
#include <memory>
#include <variant>

#include <tl/expected.hpp>

#include "api/api_rest_error.hpp"
#include "modules/dynamic/api.hpp"

namespace swagger::v1::model {
class MemoryGenerator;
class MemoryGeneratorConfig;
class MemoryGeneratorResult;
class MemoryInfoResult;
} // namespace swagger::v1::model

namespace openperf::message {
struct serialized_message;
}

namespace openperf::memory::generator {
class coordinator;
class result;
struct config;
} // namespace openperf::memory::generator

namespace openperf::memory::api {

static constexpr auto endpoint = "inproc://openperf_memory";

using serialized_msg = openperf::message::serialized_message;

using mem_generator_type = swagger::v1::model::MemoryGenerator;
using mem_generator_ptr = std::unique_ptr<mem_generator_type>;

using mem_generator_result_type = swagger::v1::model::MemoryGeneratorResult;
using mem_generator_result_ptr = std::unique_ptr<mem_generator_result_type>;

using mem_info_type = swagger::v1::model::MemoryInfoResult;
using mem_info_ptr = std::unique_ptr<mem_info_type>;

enum class pattern_type { none = 0, random, sequential, reverse };
pattern_type to_pattern_type(std::string_view name);
std::string to_string(pattern_type type);

/**
 * Memory server requests
 */

namespace request {

namespace detail {

struct empty_message
{};

struct id_message
{
    std::string id;
};

struct ids_message
{
    std::vector<std::string> ids;
};

} // namespace detail

namespace generator {

struct list : detail::empty_message
{};

struct get : detail::id_message
{};

struct erase : detail::id_message
{};

struct create
{
    mem_generator_ptr generator;
};

struct start
{
    std::string id;
    dynamic::configuration dynamic_results;
};

struct stop : detail::id_message
{};

namespace bulk {

struct create
{
    std::vector<mem_generator_ptr> generators;
};

struct erase : detail::ids_message
{};

struct start
{
    std::vector<std::string> ids;
    dynamic::configuration dynamic_results;
};

struct stop : detail::ids_message
{};

} // namespace bulk

} // namespace generator

namespace result {

struct list : detail::empty_message
{};

struct get : detail::id_message
{};

struct erase : detail::id_message
{};

} // namespace result

} // namespace request

/**
 * Memory server replies
 */

namespace reply {

struct generators
{
    std::vector<mem_generator_ptr> generators;
};

struct results
{
    std::vector<mem_generator_result_ptr> results;
};

struct ok
{};

using error_type = openperf::api::rest::error_type;
using typed_error = openperf::api::rest::typed_error;

struct error
{
    typed_error info;
};

} // namespace reply

/**
 * Memory server messages
 */

using request_msg = std::variant<request::generator::list,
                                 request::generator::get,
                                 request::generator::create,
                                 request::generator::erase,
                                 request::generator::start,
                                 request::generator::stop,
                                 request::generator::bulk::create,
                                 request::generator::bulk::erase,
                                 request::generator::bulk::start,
                                 request::generator::bulk::stop,
                                 request::result::list,
                                 request::result::get,
                                 request::result::erase>;

using reply_msg =
    std::variant<reply::generators, reply::results, reply::ok, reply::error>;

serialized_msg serialize(request_msg&& request);
serialized_msg serialize(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

mem_info_ptr get_memory_info();

bool is_valid(const mem_generator_type&, std::vector<std::string>&);
bool is_valid(const swagger::v1::model::DynamicResultsConfig&,
              std::vector<std::string>&);

mem_generator_ptr to_swagger(std::string_view id,
                             const generator::coordinator& gen);
mem_generator_result_ptr
to_swagger(std::string_view generator_id,
           std::string_view result_id,
           const std::shared_ptr<generator::result>& result);

generator::config
from_swagger(const swagger::v1::model::MemoryGeneratorConfig&);

} // namespace openperf::memory::api

#endif // _OP_MEMORY_API_HPP_
