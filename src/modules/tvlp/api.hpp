#ifndef _OP_TVLP_API_HPP_
#define _OP_TVLP_API_HPP_

#include <chrono>
#include <string>
#include <variant>

#include <json.hpp>
#include <tl/expected.hpp>
#include <zmq.h>

#include "models/tvlp_config.hpp"
#include "models/tvlp_result.hpp"

namespace openperf::message {
struct serialized_message;
};

namespace openperf::tvlp::api {

static constexpr auto endpoint = "inproc://openperf_tvlp";

using realtime = timesync::chrono::realtime;
using time_point = realtime::time_point;
using serialized_msg = openperf::message::serialized_message;

struct message
{};
struct id_message : message
{
    std::string id;
};

namespace request {

namespace tvlp {

struct list : message
{};
struct get : id_message
{};
struct erase : id_message
{};

using create = model::tvlp_configuration_t;

struct start : message
{
    std::string id;
    time_point start_time;
};
struct stop : id_message
{};

namespace result {
struct list : message
{};
struct get : id_message
{};
struct erase : id_message
{};

} // namespace result
} // namespace tvlp
} // namespace request

namespace reply {

struct ok : message
{};

struct error : message
{
    enum type_t : uint8_t {
        NONE = 0,
        NOT_FOUND,
        EXISTS,
        INVALID_ID,
        ZMQ_ERROR,
        BAD_REQUEST_ERROR
    };

    type_t type = NONE;
    int code = 0;
    std::string message;
};

namespace tvlp {

using item = model::tvlp_configuration_t;
using list = std::vector<item>;

namespace result {

using item = model::tvlp_result_t;
using list = std::vector<item>;

} // namespace result
} // namespace tvlp
} // namespace reply

// Variant types
using api_request = std::variant<request::tvlp::list,
                                 request::tvlp::get,
                                 request::tvlp::erase,
                                 request::tvlp::create,
                                 request::tvlp::start,
                                 request::tvlp::stop,
                                 request::tvlp::result::list,
                                 request::tvlp::result::get,
                                 request::tvlp::result::erase>;

using api_reply = std::variant<reply::ok,
                               reply::error,
                               reply::tvlp::list,
                               reply::tvlp::item,
                               reply::tvlp::result::item,
                               reply::tvlp::result::list>;

serialized_msg serialize(api_request&& request);
serialized_msg serialize(api_reply&& reply);

tl::expected<api_request, int> deserialize_request(serialized_msg&& msg);
tl::expected<api_reply, int> deserialize_reply(serialized_msg&& msg);

} // namespace openperf::tvlp::api

#endif // _OP_TVLP_API_HPP_
