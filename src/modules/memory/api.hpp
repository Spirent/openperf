#ifndef _OP_MEMORY_API_HPP_
#define _OP_MEMORY_API_HPP_

#include <string>
#include <memory>
#include <variant>

#include <tl/expected.hpp>
#include <zmq.h>

#include "generator.hpp"
#include "info.hpp"

namespace openperf::memory {
struct serialized_message;
}

namespace openperf::memory::api {

static constexpr auto endpoint = "inproc://openperf_memory";

// ZMQ message structs
using config_t = openperf::memory::internal::generator::config_t;
using serialized_msg = openperf::message::serialized_message;

struct message
{};
struct id_message : message
{
    std::string id;
};

namespace request {

struct info : message
{};

namespace generator {

struct list : message
{};
struct get : id_message
{};
struct erase : id_message
{};
struct create
{
    std::string id;
    bool is_running;
    config_t config;
};
struct stop : id_message
{};
struct start
{
    std::string id;
    dynamic::configuration dynamic_results;
};

namespace bulk {

using id_list = std::vector<std::string>;
using create = std::vector<generator::create>;

struct start
{
    id_list ids;
    dynamic::configuration dynamic_results;
};

struct stop : id_list
{};

struct erase : id_list
{};

} // namespace bulk
} // namespace generator

namespace statistic {
struct list : message
{};
struct get : id_message
{};
struct erase : id_message
{};
} // namespace statistic
} // namespace request

namespace reply {

struct ok : message
{};
struct error
{
    enum type_t {
        NONE = 0,
        ACTIVE_STAT,
        NOT_FOUND,
        EXISTS,
        INVALID_ID,
        NOT_INITIALIZED,
        ZMQ_ERROR,
        CUSTOM
    };

    type_t type = NONE;
    int value = 0;
    std::string message;
};

using info = memory_info::info_t;

namespace generator {
struct item
{
    std::string id;
    bool is_running;
    config_t config;
    int32_t init_percent_complete;
};

using list = std::vector<item>;

} // namespace generator

namespace statistic {
struct item
{
    std::string id;
    std::string generator_id;
    internal::memory_stat stat;
    dynamic::results dynamic_results;
};

using list = std::vector<item>;

} // namespace statistic
} // namespace reply

// Variant types
using api_request = std::variant<request::info,
                                 request::generator::list,
                                 request::generator::get,
                                 request::generator::erase,
                                 request::generator::create,
                                 request::generator::bulk::create,
                                 request::generator::bulk::erase,
                                 request::generator::start,
                                 request::generator::stop,
                                 request::generator::bulk::start,
                                 request::generator::bulk::stop,
                                 request::statistic::list,
                                 request::statistic::get,
                                 request::statistic::erase>;

using api_reply = std::variant<reply::ok,
                               reply::error,
                               reply::info,
                               reply::generator::list,
                               reply::generator::item,
                               reply::statistic::list,
                               reply::statistic::item>;

serialized_msg serialize(api_request&& request);
serialized_msg serialize(api_reply&& reply);

tl::expected<api_request, int> deserialize_request(serialized_msg&& msg);
tl::expected<api_reply, int> deserialize_reply(serialized_msg&& msg);

} // namespace openperf::memory::api

#endif // _OP_MEMORY_API_HPP_
