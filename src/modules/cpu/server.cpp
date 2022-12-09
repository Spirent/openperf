#include <cstring>

#include <zmq.h>

#include "cpu/server.hpp"
#include "message/serialized_message.hpp"
#include "utils/overloaded_visitor.hpp"

#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuGeneratorResult.h"

namespace openperf::cpu::api {

static std::string to_string(const request_msg& request)
{
    return (
        std::visit(utils::overloaded_visitor(
                       [](const request_cpu_generator_list&) {
                           return (std::string("list cpu generators"));
                       },
                       [](const request_cpu_generator& request) {
                           return ("get cpu generator " + request.id);
                       },
                       [](const request_cpu_generator_add&) {
                           return (std::string("create cpu generator"));
                       },
                       [](const request_cpu_generator_del& request) {
                           return ("delete cpu generator " + request.id);
                       },
                       [](const request_cpu_generator_bulk_add&) {
                           return (std::string("bulk create cpu generators"));
                       },
                       [](const request_cpu_generator_bulk_del&) {
                           return (std::string("bulk delete cpu generators"));
                       },
                       [](const request_cpu_generator_start& request) {
                           return ("start cpu generator " + request.id);
                       },
                       [](const request_cpu_generator_stop& request) {
                           return ("stop cpu generator " + request.id);
                       },
                       [](const request_cpu_generator_bulk_start&) {
                           return (std::string("bulk start cpu generators"));
                       },
                       [](const request_cpu_generator_bulk_stop&) {
                           return (std::string("bulk stop cpu generators"));
                       },
                       [](const request_cpu_generator_result_list&) {
                           return (std::string("list cpu generator results"));
                       },
                       [](const request_cpu_generator_result& request) {
                           return ("get cpu generator result " + request.id);
                       },
                       [](const request_cpu_generator_result_del& request) {
                           return ("delete cpu generator result " + request.id);
                       }),
                   request));
}

static std::string to_string(const reply_msg& reply)
{
    if (auto error = std::get_if<reply_error>(&reply)) {
        return ("failed: " + std::string(strerror(error->info.value)));
    }

    return ("succeeded");
}

static int handle_rpc_request(const op_event_data* data, void* arg)
{
    auto s = reinterpret_cast<server*>(arg);

    auto reply_errors = 0;
    while (auto request = message::recv(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {

        OP_LOG(OP_LOG_TRACE,
               "Received request to %s\n",
               to_string(*request).c_str());

        auto reply = std::visit(
            [&](auto& request) -> reply_msg {
                return (s->handle_request(request));
            },
            *request);

        OP_LOG(OP_LOG_TRACE,
               "Request to %s %s\n",
               to_string(*request).c_str(),
               to_string(reply).c_str());

        if (message::send(data->socket, serialize_reply(std::move(reply)))
            == -1) {
            reply_errors++;
            OP_LOG(
                OP_LOG_ERROR, "Error sending reply: %s\n", zmq_strerror(errno));
            continue;
        }
    }

    return ((reply_errors || errno == ETERM) ? -1 : 0);
}

/**
 * Implementation
 **/

server::server(void* context, core::event_loop& loop)
    : m_context(context)
    , m_loop(loop)
    , m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
{
    auto callbacks = op_event_callbacks{.on_read = handle_rpc_request};
    m_loop.add(m_socket.get(), &callbacks, this);
}

reply_msg server::handle_request(const request_cpu_generator_list&)
{
    auto reply = reply_cpu_generators{};

    std::transform(std::begin(m_coordinators),
                   std::end(m_coordinators),
                   std::back_inserter(reply.generators),
                   [](const auto& pair) {
                       return (to_swagger(pair.first, *pair.second));
                   });

    return (reply);
}

reply_msg server::handle_request(const request_cpu_generator& request)
{
    if (auto cursor = m_coordinators.find(request.id);
        cursor != std::end(m_coordinators)) {
        auto reply = reply_cpu_generators{};
        reply.generators.emplace_back(
            to_swagger(cursor->first, *cursor->second));
        return (reply);
    }

    return (to_error(error_type::NOT_FOUND));
}

reply_msg server::handle_request(const request_cpu_generator_add& request)
{
    /* if the user did not specify an id, create one for them. */
    auto id = request.generator->getId().empty()
                  ? to_string(core::uuid::random())
                  : request.generator->getId();

    if (m_coordinators.count(id) != 0) {
        return (to_error(error_type::POSIX, EEXIST));
    }

    auto [cursor, success] = m_coordinators.emplace(
        id,
        std::make_unique<generator::coordinator>(
            m_context, from_swagger(*request.generator->getConfig())));
    assert(success);

    auto reply = reply_cpu_generators{};
    reply.generators.emplace_back(to_swagger(cursor->first, *cursor->second));
    return (reply);
}

reply_msg server::handle_request(const request_cpu_generator_del& request)
{
    auto cursor = m_coordinators.find(request.id);
    if (cursor == std::end(m_coordinators)) {
        return (to_error(error_type::NOT_FOUND));
    }

    if (cursor->second->active()) {
        return (to_error(error_type::POSIX, EBUSY));
    }

    /* Delete all associated result objects */
    auto range = m_coordinator_results.equal_range(request.id);
    std::for_each(range.first, range.second, [&](const auto& pair) {
        m_results.erase(pair.second);
    });
    m_coordinator_results.erase(range.first, range.second);

    /* Delete this coordinator */
    m_coordinators.erase(cursor);

    return (reply_ok{});
}

reply_msg server::handle_request(const request_cpu_generator_bulk_add& request)
{
    auto bulk_reply = reply_cpu_generators{};
    auto bulk_errors = std::vector<reply_error>{};

    std::for_each(
        std::begin(request.generators),
        std::end(request.generators),
        [&](const auto& generator) {
            auto api_reply = handle_request(request_cpu_generator_add{
                std::make_unique<cpu_generator_type>(*generator)});
            if (auto reply = std::get_if<reply_cpu_generators>(&api_reply)) {
                assert(reply->generators.size() == 1);
                bulk_reply.generators.emplace_back(
                    std::move(reply->generators[0]));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        /* Roll back! */
        std::for_each(std::begin(bulk_reply.generators),
                      std::end(bulk_reply.generators),
                      [&](const auto& generator) {
                          handle_request(
                              request_cpu_generator_del{generator->getId()});
                      });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_cpu_generator_bulk_del& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_cpu_generator_del{id});
        });

    return (reply_ok{});
}

template <typename Map> static core::uuid get_unique_result_id(const Map& map)
{
    auto id = core::uuid::random();
    while (map.count(id)) { id = core::uuid::random(); }
    return (id);
}

reply_msg server::handle_request(const request_cpu_generator_start& request)
{
    auto found = m_coordinators.find(request.id);
    if (found == std::end(m_coordinators)) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto& generator_id = found->first;
    auto& generator = found->second;

    if (generator->active()) { return (to_error(error_type::POSIX, EINVAL)); }

    /* Generator exists and is ready  */
    auto [cursor, success] =
        m_results.emplace(get_unique_result_id(m_results),
                          generator->start(m_loop, request.dynamic_results));
    assert(success);

    auto& result_id = cursor->first;
    auto& result = cursor->second;

    /* Update coordinator <-> result mapping */
    m_coordinator_results.emplace(generator_id, result_id);

    auto reply = reply_cpu_generator_results{};
    reply.results.emplace_back(
        to_swagger(generator_id, to_string(result_id), result));
    return (reply);
}

reply_msg server::handle_request(const request_cpu_generator_stop& request)
{
    if (auto cursor = m_coordinators.find(request.id);
        cursor != std::end(m_coordinators)) {
        auto& generator = cursor->second;
        generator->stop(m_loop);
    }

    return (reply_ok{});
}

reply_msg
server::handle_request(const request_cpu_generator_bulk_start& request)
{
    auto bulk_reply = reply_cpu_generator_results{};
    auto bulk_errors = std::vector<reply_error>{};

    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            auto api_reply = handle_request(
                request_cpu_generator_start{id, request.dynamic_results});
            if (auto reply =
                    std::get_if<reply_cpu_generator_results>(&api_reply)) {
                assert(reply->results.size() == 1);
                bulk_reply.results.emplace_back(std::move(reply->results[0]));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        std::for_each(std::begin(bulk_reply.results),
                      std::end(bulk_reply.results),
                      [&](const auto& result) {
                          handle_request(request_cpu_generator_stop{
                              result->getGeneratorId()});
                          handle_request(request_cpu_generator_result_del{
                              result->getId()});
                      });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_cpu_generator_bulk_stop& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_cpu_generator_stop{id});
        });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_cpu_generator_result_list&)
{
    auto reply = reply_cpu_generator_results{};

    std::transform(std::begin(m_coordinator_results),
                   std::end(m_coordinator_results),
                   std::back_inserter(reply.results),
                   [&](const auto& pair) {
                       assert(m_results.count(pair.second) == 1);
                       return (to_swagger(pair.first,
                                          to_string(pair.second),
                                          m_results[pair.second]));
                   });

    return (reply);
}

template <typename Map>
std::optional<typename Map::key_type>
find_key(const Map& map, const typename Map::mapped_type& value)
{
    auto cursor =
        std::find_if(std::begin(map), std::end(map), [&](const auto& pair) {
            return (pair.second == value);
        });

    if (cursor == std::end(map)) { return (std::nullopt); }

    return (cursor->first);
}

static tl::expected<core::uuid, reply_error> to_uuid(std::string_view id)
{
    try {
        auto uuid = core::uuid(id);
        return (uuid);
    } catch (const std::runtime_error&) {
        return (tl::make_unexpected(to_error(error_type::NOT_FOUND)));
    }
}

reply_msg server::handle_request(const request_cpu_generator_result& request)
{
    auto id = to_uuid(request.id);
    if (!id) { return (id.error()); }

    auto cursor = m_results.find(id.value());
    if (cursor == std::end(m_results)) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_cpu_generator_results{};
    reply.results.emplace_back(
        to_swagger(find_key(m_coordinator_results, cursor->first).value(),
                   request.id,
                   cursor->second));
    return (reply);
}

template <typename Container, typename Predicate>
void erase_if(Container& c, Predicate&& p)
{
    auto cursor = std::begin(c);
    auto end = std::end(c);

    while (cursor != end) {
        if (p(*cursor)) {
            cursor = c.erase(cursor);
        } else {
            ++cursor;
        }
    }
}

reply_msg
server::handle_request(const request_cpu_generator_result_del& request)
{
    auto id = to_uuid(request.id);
    if (!id) { return (id.error()); }

    auto cursor = m_results.find(id.value());
    if (cursor == std::end(m_results)) {
        return (to_error(error_type::NOT_FOUND));
    }

    /*
     * We can't delete this result if it's actively in use by
     * a coordinator. Unfortunately, we don't explicitly store that
     * data so we have to jump through some hoops...
     * Note: the coordinator must exist because results can only
     * be created from coordinators and results cannot live past the
     * lifetime of their parent coordinator.
     */
    auto coord = m_coordinators.find(
        find_key(m_coordinator_results, cursor->first).value());
    assert(coord != std::end(m_coordinators));
    if (cursor->second.get() == coord->second->result()) {
        return (to_error(error_type::POSIX, EBUSY));
    }

    m_results.erase(cursor);
    erase_if(m_coordinator_results,
             [&](const auto& pair) { return (pair.second == id.value()); });

    return (reply_ok{});
}

} // namespace openperf::cpu::api
