#ifndef _OP_MEMORY_API_HPP_
#define _OP_MEMORY_API_HPP_

#include <string>
#include <memory>
#include <variant>

#include <json.hpp>
#include <tl/expected.hpp>
#include <zmq.h>

#include "info.hpp"
#include "generator.hpp"
#include "memory_stat.hpp"

#include "framework/dynamic/api.hpp"

namespace swagger::v1::model {
class MemoryGenerator;
class BulkCreateMemoryGeneratorsRequest;
class BulkDeleteMemoryGeneratorsRequest;
class BulkStartMemoryGeneratorsRequest;
class BulkStopMemoryGeneratorsRequest;

void from_json(const nlohmann::json&, MemoryGenerator&);
void from_json(const nlohmann::json&, BulkCreateMemoryGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkDeleteMemoryGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStartMemoryGeneratorsRequest&);
void from_json(const nlohmann::json&, BulkStopMemoryGeneratorsRequest&);
} // namespace swagger::v1::model

namespace openperf::memory::api {

static constexpr auto endpoint = "inproc://openperf_memory";

// ZMQ message structs
using config_t = openperf::memory::internal::generator::config_t;

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

using config_ptr = std::unique_ptr<config_t>;
struct create_data
{
    std::string id;
    bool is_running;
    config_t config;
};
struct list : message
{};
struct get : id_message
{};
struct erase : id_message
{};
struct create
{
    std::unique_ptr<create_data> data;
};
struct stop : id_message
{};
struct start
{
    struct start_data
    {
        std::string id;
        dynamic::configuration dynamic_results;
    };

    std::unique_ptr<start_data> data;
};

namespace bulk {

struct id_list
{
    std::unique_ptr<std::vector<std::string>> data;
};

struct start
{
    struct start_data
    {
        std::vector<std::string> ids;
        dynamic::configuration dynamic_results;
    };

    std::unique_ptr<start_data> data;
};

struct stop : id_list
{};

struct erase : id_list
{};

struct create
{
    std::unique_ptr<std::vector<create_data>> data;
};

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

    struct error_data
    {
        type_t type = NONE;
        int value = 0;
        std::string message;
    };

    std::unique_ptr<error_data> data;
};
using info = memory_info::info_t;

namespace generator {
struct item
{
    struct item_data
    {
        std::string id;
        bool is_running;
        config_t config;
        int32_t init_percent_complete;
    };

    std::unique_ptr<item_data> data;
};

struct list
{
    std::unique_ptr<std::vector<item::item_data>> data;
};

} // namespace generator

namespace statistic {
struct item
{
    struct item_data
    {
        std::string id;
        std::string generator_id;
        internal::memory_stat stat;
        dynamic::results dynamic_results;
    };

    using data_ptr = std::unique_ptr<item_data>;
    data_ptr data;
};

struct list
{
    std::unique_ptr<std::vector<item::item_data>> data;
};

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

struct serialized_msg
{
    zmq_msg_t type;
    zmq_msg_t data;
};

int send_message(void* socket, serialized_msg&& msg);
tl::expected<serialized_msg, int> recv_message(void* socket, int flags = 0);

serialized_msg serialize(api_request&& request);
serialized_msg serialize(api_reply&& reply);

tl::expected<api_request, int> deserialize_request(const serialized_msg& msg);
tl::expected<api_reply, int> deserialize_reply(const serialized_msg& msg);

} // namespace openperf::memory::api

#endif // _OP_MEMORY_API_HPP_
