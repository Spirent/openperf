#ifndef _OP_NETWORK_API_HPP_
#define _OP_NETWORK_API_HPP_

#include <cinttypes>
#include <memory>
#include <string>
#include <variant>

#include <tl/expected.hpp>
#include <zmq.h>

#include "modules/dynamic/api.hpp"
#include "modules/timesync/chrono.hpp"

#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::message {
struct serialized_message;
}

namespace openperf::network::api {

using serialized_msg = openperf::message::serialized_message;

const std::string endpoint = "inproc://openperf_network";

/**
 * List of supported server requests.
 */

/* zmq api objects models */

using network_generator_t = model::generator;
using network_generator_result_t = model::generator_result;
using network_generator_ptr = std::unique_ptr<network_generator_t>;
using network_generator_result_ptr =
    std::unique_ptr<network_generator_result_t>;
using id_ptr = std::unique_ptr<std::string>;

enum class error_type { NONE = 0, NOT_FOUND, ZMQ_ERROR, CUSTOM_ERROR };

struct reply_error
{
    error_type type = error_type::NONE;
    int code = 0;
    std::string value;
};

/* zmq api request models */

struct message
{};
struct id_message : message
{
    std::string id;
};
namespace request {
namespace generator {
struct list : message
{};
struct get : id_message
{};
struct erase : id_message
{};
struct create
{
    network_generator_ptr source;
};

struct start
{
    std::string id;
    dynamic::configuration dynamic_results;
};
struct stop : id_message
{};

namespace bulk {
struct create
{
    std::vector<network_generator_ptr> generators;
};

struct erase
{
    std::vector<id_ptr> ids;
};

struct start
{
    std::vector<std::string> ids;
    dynamic::configuration dynamic_results;
};

struct stop
{
    std::vector<id_ptr> ids;
};

} // namespace bulk
} // namespace generator
namespace statistic {
struct list
{};

struct get : id_message
{};

struct erase : id_message
{};
} // namespace statistic
} // namespace request

/* zmq api reply models */

namespace reply {
namespace generator {
struct list
{
    std::vector<network_generator_ptr> generators;
};
} // namespace generator
namespace statistic {
struct list
{
    std::vector<network_generator_result_ptr> results;
};
} // namespace statistic

struct ok
{};

struct error
{
    std::unique_ptr<reply_error> info;
};
} // namespace reply
using request_msg = std::variant<request::generator::list,
                                 request::generator::get,
                                 request::generator::create,
                                 request::generator::erase,
                                 request::generator::bulk::create,
                                 request::generator::bulk::erase,
                                 request::generator::start,
                                 request::generator::stop,
                                 request::generator::bulk::start,
                                 request::generator::bulk::stop,
                                 request::statistic::list,
                                 request::statistic::get,
                                 request::statistic::erase>;

using reply_msg = std::variant<reply::generator::list,
                               reply::statistic::list,
                               reply::ok,
                               reply::error>;

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

std::string to_string(const api::reply_error& error);
reply::error
to_error(error_type type, int code = 0, const std::string& value = "");

} // namespace openperf::network::api

#endif /* _OP_NETWORK_API_HPP_ */
