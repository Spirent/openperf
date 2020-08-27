#ifndef _OP_PACKETIO_PORT_API_HPP_
#define _OP_PACKETIO_PORT_API_HPP_

#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "json.hpp"
#include "tl/expected.hpp"

namespace swagger::v1::model {
class Port;
}

namespace openperf {

namespace core {
class event_loop;
}

namespace message {
struct serialized_message;
}

namespace packetio::port::api {

inline constexpr std::string_view endpoint = "inproc://op_packetio_port";

using port_type = swagger::v1::model::Port;
using port_ptr = std::unique_ptr<port_type>;

enum class filter_key_type { none, kind };
using filter_map_type = std::map<filter_key_type, std::string>;
using filter_map_ptr = std::unique_ptr<filter_map_type>;

using serialized_msg = openperf::message::serialized_message;

/*
 * Provide a mechanism for associating type information with
 * expected REST query strings.
 * This is probably overkill for two filter types, but we
 * should have more later.
 */
template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

constexpr auto filter_key_names =
    associative_array<std::string_view, filter_key_type>(
        std::pair("kind", filter_key_type::kind));

constexpr filter_key_type to_key_type(std::string_view name)
{
    auto cursor = std::begin(filter_key_names),
         end = std::end(filter_key_names);
    while (cursor != end) {
        if (cursor->first == name) return (cursor->second);
        cursor++;
    }

    return (filter_key_type::none);
}

constexpr std::string_view to_key_name(filter_key_type key)
{
    auto cursor = std::begin(filter_key_names),
         end = std::end(filter_key_names);
    while (cursor != end) {
        if (cursor->second == key) return (cursor->first);
        cursor++;
    }

    return ("unknown");
}

struct request_list_ports
{
    filter_map_ptr filter;
};

struct request_create_port
{
    port_ptr port;
};

struct request_get_port
{
    std::string id;
};

struct request_update_port
{
    port_ptr port;
};

struct request_delete_port
{
    std::string id;
};

using request_msg = std::variant<request_list_ports,
                                 request_create_port,
                                 request_get_port,
                                 request_update_port,
                                 request_delete_port>;
struct reply_ok
{};

enum class error_type { NONE = 0, NOT_FOUND, VERBOSE };

struct reply_error
{
    error_type type = error_type::NONE;
    std::string msg;
};

struct reply_ports
{
    std::vector<port_ptr> ports;
};

using reply_msg = std::variant<reply_ports, reply_ok, reply_error>;

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

bool is_valid(const swagger::v1::model::Port&, std::vector<std::string>&);

} // namespace packetio::port::api
} // namespace openperf

namespace swagger::v1::model {
void from_json(const nlohmann::json&, swagger::v1::model::Port&);
}

#endif /*_OP_PACKETIO_PORT_API_HPP_ */
