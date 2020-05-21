#include <zmq.h>

#include "packet/generator/server.hpp"
#include "utils/overloaded_visitor.hpp"

#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"
#include "swagger/v1/model/TxFlow.h"

namespace openperf::packet::generator::api {

/**
 * Utility functions and templates
 */

template <typename Container, typename Condition>
static void erase_if(Container& c, Condition&& op)
{
    auto cursor = std::begin(c);
    while (cursor != std::end(c)) {
        if (op(*cursor)) {
            cursor = c.erase(cursor);
        } else {
            ++cursor;
        }
    }
}

template <typename InputIt,
          typename OutputIt,
          typename FilterFn,
          typename TransformFn>
OutputIt transform_if(InputIt first,
                      InputIt last,
                      OutputIt cursor,
                      FilterFn&& do_xform,
                      TransformFn&& xform)
{
    std::for_each(first, last, [&](const auto& item) {
        if (do_xform(item)) { cursor++ = xform(item); }
    });

    return (cursor);
}

struct source_id_comparator
{
    bool operator()(const packetio::packet::generic_source& left,
                    std::string_view right)
    {
        return (left.id() < right);
    }

    bool operator()(std::string_view left,
                    const packetio::packet::generic_source& right)
    {
        return (left < right.id());
    }
};

template <typename Iterator, typename T, typename Compare = std::less<>>
Iterator
binary_find(Iterator first, Iterator last, const T& value, Compare comp = {})
{
    auto result = std::lower_bound(first, last, value, comp);
    return (result != last && !comp(value, *result) ? result : last);
}

static std::optional<core::uuid> to_uuid(std::string_view name)
{
    try {
        return (core::uuid(name));
    } catch (...) {
        return (std::nullopt);
    }
}

static std::string to_string(const request_msg& request)
{
    return (std::visit(utils::overloaded_visitor(
                           [](const request_list_generators&) {
                               return (std::string("list generators"));
                           },
                           [](const request_create_generator&) {
                               return (std::string("create generator"));
                           },
                           [](const request_delete_generators&) {
                               return (std::string("delete generators"));
                           },
                           [](const request_get_generator& request) {
                               return ("get generator " + request.id);
                           },
                           [](const request_delete_generator& request) {
                               return ("delete generator " + request.id);
                           },
                           [](const request_start_generator& request) {
                               return ("start generator " + request.id);
                           },
                           [](const request_stop_generator& request) {
                               return ("stop generator " + request.id);
                           },
                           [](const request_list_generator_results&) {
                               return (std::string("list generator results"));
                           },
                           [](const request_delete_generator_results&) {
                               return (std::string("delete generator results"));
                           },
                           [](const request_get_generator_result& request) {
                               return ("get generator result " + request.id);
                           },
                           [](const request_delete_generator_result& request) {
                               return ("delete generator result " + request.id);
                           },
                           [](const request_list_tx_flows&) {
                               return (std::string("list tx flows"));
                           },
                           [](const request_get_tx_flow& request) {
                               return ("get tx flow " + request.id);
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

int handle_rpc_request(const op_event_data* data, void* arg)
{
    auto s = reinterpret_cast<server*>(arg);

    auto reply_errors = 0;
    while (auto request = recv_message(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        OP_LOG(OP_LOG_TRACE,
               "Received request to %s\n",
               to_string(*request).c_str());

        auto request_visitor = [&](auto& request) -> reply_msg {
            return (s->handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);

        OP_LOG(OP_LOG_TRACE,
               "Request to %s %s\n",
               to_string(*request).c_str(),
               to_string(reply).c_str());

        if (send_message(data->socket, serialize_reply(std::move(reply)))
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
 */
server::server(void* context, core::event_loop& loop)
    : m_loop(loop)
    , m_client(context)
    , m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
{
    /* Setup event loop */
    auto callbacks = op_event_callbacks{.on_read = handle_rpc_request};
    m_loop.add(m_socket.get(), &callbacks, this);
}

reply_msg server::handle_request(const request_list_generators& request)
{
    auto reply = reply_generators{};

    if (request.filter && request.filter->count(filter_type::target_id)) {
        auto& target = (*request.filter)[filter_type::target_id];
        transform_if(
            std::begin(m_sources),
            std::end(m_sources),
            std::back_inserter(reply.generators),
            [&](const auto& item) {
                return (item.template get<source>().target() == target);
            },
            [](const auto& item) {
                return (to_swagger(item.template get<source>()));
            });
    } else {
        /* return all generators */
        std::transform(std::begin(m_sources),
                       std::end(m_sources),
                       std::back_inserter(reply.generators),
                       [&](const auto& item) {
                           return (to_swagger(item.template get<source>()));
                       });
    }

    return (reply);
}

reply_msg server::handle_request(const request_create_generator& request)
{
    auto config = source_config{.target = request.generator->getTargetId(),
                                .api_config = request.generator->getConfig()};

    if (!request.generator->getId().empty()) {
        config.id = request.generator->getId();
    }

    /* Check if id already exists */
    if (std::binary_search(std::begin(m_sources),
                           std::end(m_sources),
                           config.id,
                           source_id_comparator{})) {
        return (to_error(error_type::POSIX, EEXIST));
    }

    /* Verify target id exists */
    auto tx_ids = m_client.get_worker_tx_ids(config.target);
    if (!tx_ids || tx_ids->empty()) {
        return (to_error(error_type::POSIX, EINVAL));
    }

    auto& item = m_sources.emplace_back(source(std::move(config)));

    /* Try to add source to the back-end workers */
    if (auto success =
            m_client.add_source(request.generator->getTargetId(), item);
        !success) {
        /* Delete the item we just added to the back of the vector */
        m_sources.erase(std::prev(std::end(m_sources)));
        return (to_error(error_type::POSIX, success.error()));
    }

    /* Success; sort sources and return */
    std::sort(std::begin(m_sources),
              std::end(m_sources),
              [](const auto& left, const auto& right) {
                  return (left.id() < right.id());
              });

    auto reply = reply_generators{};
    reply.generators.emplace_back(to_swagger(item.template get<source>()));
    return (reply);
}

static void remove_source(packetio::internal::api::client& client,
                          packetio::packet::generic_source& to_del)
{
    if (auto success =
            client.del_source(to_del.template get<source>().target(), to_del);
        !success) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to remove generator %s from packetio workers!\n",
               to_del.id().c_str());
    }
}

reply_msg server::handle_request(const request_delete_generators&)
{
    /* Delete all inactive results */
    erase_if(m_results, [](const auto& pair) {
        return (!pair.second->parent().active());
    });

    /*
     * Sort generators into active and inactive ones.  We want to delete
     * all of the inactive ones.
     */
    auto cursor = std::stable_partition(
        std::begin(m_sources), std::end(m_sources), [](const auto& item) {
            return (item.template get<source>().active());
        });

    /* Remove sinks from our workers */
    std::for_each(cursor, std::end(m_sources), [&](auto& item) {
        remove_source(m_client, item);
    });

    /* And erase from existence */
    m_sources.erase(cursor, std::end(m_sources));

    return (reply_ok{});
}

reply_msg server::handle_request(const request_get_generator& request)
{
    auto result = binary_find(std::begin(m_sources),
                              std::end(m_sources),
                              request.id,
                              source_id_comparator{});

    if (result == std::end(m_sources)) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_generators{};
    reply.generators.emplace_back(to_swagger(result->template get<source>()));
    return (reply);
}

reply_msg server::handle_request(const request_delete_generator& request)
{
    auto result = binary_find(std::begin(m_sources),
                              std::end(m_sources),
                              request.id,
                              source_id_comparator{});

    if (result != std::end(m_sources)
        && !result->template get<source>().active()) {
        /* Delete this generators result objects */
        erase_if(m_results, [&](const auto& pair) {
            return (pair.second->parent().id() == request.id);
        });

        /* Delete this generator */
        remove_source(m_client, *result);
        m_sources.erase(std::remove(result, std::next(result), *result),
                        std::end(m_sources));
    }

    return (reply_ok{});
}

template <typename Map> static core::uuid get_unique_result_id(const Map& map)
{
    auto id = api::get_generator_result_id();
    while (map.count(id)) { id = api::get_generator_result_id(); }
    return (id);
}

reply_msg server::handle_request(const request_start_generator& request)
{
    auto found = binary_find(std::begin(m_sources),
                             std::end(m_sources),
                             request.id,
                             source_id_comparator{});

    if (found == std::end(m_sources)) {
        return (to_error(error_type::NOT_FOUND));
    }

    if (found->active()) { return (to_error(error_type::POSIX, EINVAL)); }

    auto& impl = found->template get<source>();

    auto item = m_results.emplace(get_unique_result_id(m_results),
                                  std::make_unique<source_result>(impl));
    assert(item.second); /* source_result inserted */

    auto& id = item.first->first;
    auto& result = item.first->second;

    impl.start(result.get());

    auto reply = reply_generator_results{};
    reply.generator_results.emplace_back(to_swagger(id, *result));
    return (reply);
}

reply_msg server::handle_request(const request_stop_generator& request)
{
    if (auto found = binary_find(std::begin(m_sources),
                                 std::end(m_sources),
                                 request.id,
                                 source_id_comparator{});
        found != std::end(m_sources)) {
        auto& impl = found->template get<source>();
        impl.stop();
    }

    return (reply_ok{});
}

reply_msg server::handle_request(const request_list_generator_results& request)
{
    auto reply = reply_generator_results{};

    auto compare = std::function<bool(const result_map::value_type& pair)>{};
    if (!request.filter) {
        compare = [](const auto&) { return (true); };
    } else {
        auto& filter = *request.filter;
        compare = [&](const auto& item) {
            if (filter.count(filter_type::generator_id)
                && filter[filter_type::generator_id]
                       != item.second->parent().id()) {
                return (false);
            }

            if (filter.count(filter_type::target_id)
                && filter[filter_type::target_id]
                       != item.second->parent().target()) {
                return (false);
            }

            return (true);
        };
    }

    assert(compare);

    transform_if(std::begin(m_results),
                 std::end(m_results),
                 std::back_inserter(reply.generator_results),
                 compare,
                 [](const auto& item) {
                     return (to_swagger(item.first, *item.second));
                 });

    return (reply);
}

reply_msg server::handle_request(const request_delete_generator_results&)
{
    /* Delete all inactive results */
    erase_if(m_results, [](const auto& pair) {
        return (!pair.second->parent().active());
    });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_get_generator_result& request)
{
    auto id = to_uuid(request.id);
    if (!id) { return (to_error(error_type::NOT_FOUND)); }

    auto result = m_results.find(*id);
    if (result == std::end(m_results)) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_generator_results{};
    reply.generator_results.emplace_back(
        to_swagger(result->first, *result->second));
    return (reply);
}

reply_msg server::handle_request(const request_delete_generator_result& request)
{
    if (auto id = to_uuid(request.id); id.has_value()) {
        if (auto result = m_results.find(*id); result != std::end(m_results)) {
            if (!result->second->parent().active()) { m_results.erase(*id); }
        }
    }

    return (reply_ok{});
}

reply_msg server::handle_request(const request_list_tx_flows& request)
{
    auto compare = std::function<bool(const result_map::value_type& pair)>{};
    if (!request.filter) {
        compare = [](const auto&) { return (true); };
    } else {
        auto& filter = *request.filter;
        compare = [&](const auto& item) {
            if (filter.count(filter_type::generator_id)
                && filter[filter_type::generator_id]
                       != item.second->parent().id()) {
                return (false);
            }

            if (filter.count(filter_type::target_id)
                && filter[filter_type::target_id]
                       != item.second->parent().target()) {
                return (false);
            }

            return (true);
        };
    }

    assert(compare);

    auto reply = reply_tx_flows{};

    std::for_each(std::begin(m_results),
                  std::end(m_results),
                  [&](const auto& result_pair) {
                      if (!compare(result_pair)) { return; }

                      const auto& flows = result_pair.second->counters();
                      auto offset = 0U;
                      std::generate_n(
                          std::back_inserter(reply.flows), flows.size(), [&]() {
                              auto flow_ptr = to_swagger(
                                  tx_flow_id(result_pair.first, offset),
                                  result_pair.first,
                                  *result_pair.second,
                                  offset);
                              offset++;
                              return (flow_ptr);
                          });
                  });

    return (reply);
}

reply_msg server::handle_request(const request_get_tx_flow& request)
{
    auto id = to_uuid(request.id);
    if (!id) { return (to_error(error_type::NOT_FOUND)); }

    auto [min_id, flow_idx] = tx_flow_tuple(*id);

    auto it = m_results.lower_bound(min_id);
    if (it == std::end(m_results)) { return (to_error(error_type::NOT_FOUND)); }

    const auto& result = it->second;
    if (flow_idx >= result->counters().size()) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_tx_flows{};
    reply.flows.emplace_back(to_swagger(*id, it->first, *result, flow_idx));
    return (reply);
}

} // namespace openperf::packet::generator::api
