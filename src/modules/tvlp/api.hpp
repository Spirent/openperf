#ifndef _OP_TVLP_API_HPP_
#define _OP_TVLP_API_HPP_

#include <string>
#include <variant>

#include <chrono>
#include <json.hpp>
#include <zmq.h>
#include <tl/expected.hpp>

#include "message/serialized_message.hpp"
#include "models/tvlp_config.hpp"

namespace openperf::tvlp::api {

static constexpr auto endpoint = "inproc://openperf_tvlp";

using time_point = std::chrono::time_point<timesync::chrono::realtime>;
using realtime = timesync::chrono::realtime;
using tvlp_config_t = model::tvlp_configuration_t;
using tvlp_config_ptr = std::unique_ptr<tvlp_config_t>;
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
struct create : message
{
    std::unique_ptr<tvlp_config_t> data;
};
struct stop : id_message
{};
struct start : message
{
    struct start_data
    {
        std::string id;
        time_point start_time;
    };
    std::unique_ptr<start_data> data;
};

} // namespace tvlp
} // namespace request

namespace reply {

struct ok : message
{};

struct error_data
{
    enum type_t {
        NONE = 0,
        NOT_FOUND,
        EXISTS,
        INVALID_ID,
        ZMQ_ERROR,
        BAD_REQUEST_ERROR
    } type = NONE;
    int code = 0;
    std::string value;
};

struct error : message
{
    std::unique_ptr<error_data> data;
};

namespace tvlp {
struct item : message
{
    tvlp_config_ptr data;
};

struct list : message
{
    std::unique_ptr<std::vector<tvlp_config_t>> data;
};

} // namespace tvlp

} // namespace reply

// Variant types
using api_request = std::variant<request::tvlp::list,
                                 request::tvlp::get,
                                 request::tvlp::erase,
                                 request::tvlp::create,
                                 request::tvlp::start,
                                 request::tvlp::stop>;

using api_reply =
    std::variant<reply::ok, reply::error, reply::tvlp::list, reply::tvlp::item>;

serialized_msg serialize(api_request&& request);
serialized_msg serialize(api_reply&& reply);

tl::expected<api_request, int> deserialize_request(serialized_msg&& msg);
tl::expected<api_reply, int> deserialize_reply(serialized_msg&& msg);

} // namespace openperf::tvlp::api

#endif