#ifndef _OP_TVLP_API_HPP_
#define _OP_TVLP_API_HPP_

#include <string>
#include <variant>

#include <json.hpp>
#include <zmq.h>
#include <tl/expected.hpp>

#include "generator.hpp"

namespace swagger::v1::model {
class TvlpConfiguration;

void from_json(const nlohmann::json&, TvlpConfiguration&);
} // namespace swagger::v1::model

namespace openperf::tvlp::api {

static constexpr auto endpoint = "inproc://openperf_tvlp";

struct message
{};
struct id_message : message
{
    std::string id;
};

namespace request {

struct info : message
{};

namespace tvlp {

struct create_data
{
    std::string id;
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
struct start : id_message
{};

} // namespace tvlp
} // namespace request

namespace reply {

struct ok : message
{};
struct error : message
{
    enum { NONE = 0, NOT_FOUND, EXISTS, INVALID_ID, ZMQ_ERROR } type = NONE;
    int value = 0;
};

namespace tvlp {
struct item
{
    struct item_data
    {
        std::string id;
    };

    std::unique_ptr<item_data> data;
};

struct list
{
    std::unique_ptr<std::vector<item::item_data>> data;
};

} // namespace tvlp

namespace statistic {
struct item
{
    struct item_data
    {
        std::string id;
        std::string generator_id;
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
using api_request = std::variant<message>;

using api_reply = std::variant<reply::ok, reply::error>;

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

} // namespace openperf::tvlp::api

#endif