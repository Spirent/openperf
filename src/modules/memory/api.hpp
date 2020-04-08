#ifndef _OP_MEMORY_API_HPP_
#define _OP_MEMORY_API_HPP_

#include <string>
#include <variant>
#include <json.hpp>

#include <zmq.h>
#include <tl/expected.hpp>

#include "memory/info.hpp"
#include "memory/generator.hpp"

namespace openperf::memory::api {

static constexpr auto endpoint = "inproc://openperf_memory";

// ZMQ message structs
struct serialized_msg
{
    zmq_msg_t type;
    zmq_msg_t data;
};

using config_t = openperf::memory::internal::generator::config_t;
using stat_t = openperf::memory::internal::generator::stat_t;
// using id_t = char[64];

struct id_t
{
    char id[64];

    id_t() = default;
    id_t(std::string_view str) { operator=(str); }

    id_t(const std::string& str) { operator=(str); }

    id_t& operator=(std::string_view str)
    {
        strncpy(reinterpret_cast<char*>(id), str.data(), sizeof(id));
        return *this;
    }

    id_t& operator=(const std::string& str)
    {
        strncpy(reinterpret_cast<char*>(id), str.data(), sizeof(id));
        return *this;
    }

    operator std::string_view()
    {
        return std::string_view(reinterpret_cast<char*>(id), sizeof(id));
    }

    operator std::string() const { return std::string(id); }
};

using id_list = std::vector<id_t>;

struct message
{};
struct id_message : message
{
    id_t id;
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
struct create : id_message
{
    bool is_running;
    config_t config;
};
struct stop : id_message
{};
struct start : id_message
{};

namespace bulk {
struct start : id_list
{};
struct stop : id_list
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
struct error : message
{
    enum { NONE = 0, NOT_FOUND, EXISTS, EAI_ERROR, ZMQ_ERROR } type = NONE;
    int value = 0;
};
using info = memory_info::info_t;

namespace generator {
struct item : id_message
{
    bool is_running;
    config_t config;
};
using list = std::vector<item>;

namespace bulk {
struct result
{
    id_t id;
    enum { SUCCESS = 0, NOT_FOUND, FAIL } code;
};

using list = std::vector<result>;
} // namespace bulk

} // namespace generator

namespace statistic {
struct item
    : id_message
    , stat_t
{};

using list = std::vector<item>;
} // namespace statistic
} // namespace reply

// Variant types
using api_request = std::variant<request::info,
                                 request::generator::list,
                                 request::generator::get,
                                 request::generator::erase,
                                 request::generator::create,
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
                               reply::generator::bulk::list,
                               reply::statistic::list,
                               reply::statistic::item>;

int send_message(void* socket, serialized_msg&& msg);
tl::expected<serialized_msg, int> recv_message(void* socket, int flags = 0);

serialized_msg serialize(const api_request& request);
serialized_msg serialize(const api_reply& reply);

tl::expected<api_request, int> deserialize_request(const serialized_msg& msg);
tl::expected<api_reply, int> deserialize_reply(const serialized_msg& msg);

} // namespace openperf::memory::api

#endif // _OP_MEMORY_API_HPP_
