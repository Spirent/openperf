#include <zmq.h>

#include "message/serialized_message.hpp"
#include "packet/analyzer/server.hpp"
#include "packet/bpf/bpf.hpp"

#include "swagger/v1/model/PacketAnalyzer.h"
#include "swagger/v1/model/PacketAnalyzerResult.h"
#include "swagger/v1/model/RxFlow.h"

#include "utils/overloaded_visitor.hpp"

namespace openperf::packet::analyzer::api {

/**
 * Utility structs and functions
 **/

template <typename T, size_t N>
std::string_view to_string_view(const std::array<T, N>& array)
{
    auto ptr = array.data();
    auto len = strnlen(ptr, array.size());
    return (std::string_view(ptr, len));
}

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

struct sink_id_comparator
{
    bool operator()(const packetio::packet::generic_sink& left,
                    std::string_view right)
    {
        return (left.id() < right);
    }

    bool operator()(std::string_view left,
                    const packetio::packet::generic_sink& right)
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
                           [](const request_list_analyzers&) {
                               return (std::string("list analyzers"));
                           },
                           [](const request_create_analyzer&) {
                               return (std::string("create analyzer"));
                           },
                           [](const request_delete_analyzers&) {
                               return (std::string("delete analyzers"));
                           },
                           [](const request_get_analyzer& request) {
                               return ("get analyzer " + request.id);
                           },
                           [](const request_delete_analyzer& request) {
                               return ("delete analyzer " + request.id);
                           },
                           [](const request_reset_analyzer& request) {
                               return ("reset analyzer " + request.id);
                           },
                           [](const request_start_analyzer& request) {
                               return ("start analyzer " + request.id);
                           },
                           [](const request_stop_analyzer& request) {
                               return ("stop analyzer " + request.id);
                           },
                           [](const request_bulk_create_analyzers&) {
                               return (std::string("bulk create analyzers"));
                           },
                           [](const request_bulk_delete_analyzers&) {
                               return (std::string("bulk delete analyzers"));
                           },
                           [](const request_bulk_start_analyzers&) {
                               return (std::string("bulk start analyzers"));
                           },
                           [](const request_bulk_stop_analyzers&) {
                               return (std::string("bulk stop analyzers"));
                           },
                           [](const request_list_analyzer_results&) {
                               return (std::string("list analyzer results"));
                           },
                           [](const request_delete_analyzer_results&) {
                               return (std::string("delete analyzer results"));
                           },
                           [](const request_get_analyzer_result& request) {
                               return ("get analyzer result " + request.id);
                           },
                           [](const request_delete_analyzer_result& request) {
                               return ("delete analyzer result " + request.id);
                           },
                           [](const request_list_rx_flows&) {
                               return (std::string("list rx flows"));
                           },
                           [](const request_get_rx_flow& request) {
                               return ("get rx flow " + request.id);
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

        auto request_visitor = [&](auto& request) -> reply_msg {
            return (s->handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);

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
    : m_loop(loop)
    , m_client(context)
    , m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
{
    /* Setup event loop */
    auto callbacks = op_event_callbacks{.on_read = handle_rpc_request};
    m_loop.add(m_socket.get(), &callbacks, this);
}

reply_msg server::handle_request(const request_list_analyzers& request)
{
    auto reply = reply_analyzers{};

    if (request.filter && request.filter->count(filter_key_type::source_id)) {
        auto& source = (*request.filter)[filter_key_type::source_id];
        transform_if(
            std::begin(m_sinks),
            std::end(m_sinks),
            std::back_inserter(reply.analyzers),
            [&](const auto& item) {
                return (item.template get<sink>().source() == source);
            },
            [](const auto& item) {
                return (to_swagger(item.template get<sink>()));
            });
    } else {
        /* return all analyzers */
        std::transform(std::begin(m_sinks),
                       std::end(m_sinks),
                       std::back_inserter(reply.analyzers),
                       [&](const auto& item) {
                           return (to_swagger(item.template get<sink>()));
                       });
    }

    return (reply);
}

protocol_counters_config to_protocol_counters(std::vector<std::string>& names)
{
    auto counters = protocol_counters_config{0};

    std::for_each(std::begin(names), std::end(names), [&](const auto& name) {
        counters |= packet::statistics::to_protocol_flag(name);
    });

    return (counters);
}

flow_counters_config to_flow_counters(std::vector<std::string>& names)
{
    auto counters = flow_counters_config{statistics::flow_flags::frame_count};

    std::for_each(std::begin(names), std::end(names), [&](const auto& name) {
        counters |= statistics::to_flow_flag(name);
    });

    return (counters);
}

reply_msg server::handle_request(const request_create_analyzer& request)
{
    auto config = sink_config{.source = request.analyzer->getSourceId()};

    auto user_config = request.analyzer->getConfig();
    assert(user_config);
    if (!user_config->getProtocolCounters().empty()) {
        config.protocol_counters =
            to_protocol_counters(user_config->getProtocolCounters());
    }
    if (!user_config->getFlowCounters().empty()) {
        config.flow_counters = to_flow_counters(user_config->getFlowCounters());
    }
    if (!request.analyzer->getId().empty()) {
        config.id = request.analyzer->getId();
    }
    if (user_config->filterIsSet()) {
        config.filter = user_config->getFilter();
    }

    /* Check if id already exists in map */
    if (std::binary_search(std::begin(m_sinks),
                           std::end(m_sinks),
                           config.id,
                           sink_id_comparator{})) {
        return (to_error(error_type::POSIX, EEXIST));
    }

    auto rx_ids = m_client.get_worker_rx_ids(config.source);
    if (!rx_ids || rx_ids->empty()) {
        return (to_error(error_type::POSIX, EINVAL));
    }

    auto& item = m_sinks.emplace_back(sink(config, *rx_ids));

    /* Try to add the new sink to the backend workers */
    auto success = m_client.add_sink(
        packetio::packet::traffic_direction::RX, config.source, item);
    if (!success) {
        /*
         * Luckily, we failed adding the last item in the vector,
         * so just delete it.
         */
        m_sinks.erase(std::prev(std::end(m_sinks)));
        return (to_error(error_type::POSIX, success.error()));
    }

    /* Grab a reference before we invalidate the iterator */
    const auto& impl = item.template get<sink>();

    /* Success; sort our sinks and return */
    std::sort(std::begin(m_sinks),
              std::end(m_sinks),
              [](const auto& left, const auto& right) {
                  return (left.id() < right.id());
              });

    auto reply = reply_analyzers{};
    reply.analyzers.emplace_back(to_swagger(impl));
    return (reply);
}

static void remove_sink(packetio::internal::api::client& client,
                        packetio::packet::generic_sink& to_del)
{
    if (auto success = client.del_sink(packetio::packet::traffic_direction::RX,
                                       to_del.template get<sink>().source(),
                                       to_del);
        !success) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to remove analyzer %s from packetio workers!\n",
               to_del.id().c_str());
    }
}

reply_msg server::handle_request(const request_delete_analyzers&)
{
    /* Delete all inactive results */
    erase_if(m_results, [](const auto& pair) {
        return (!pair.second->parent().active());
    });

    /*
     * Sort analyzers into active and inactive ones.  We want to delete
     * all of the inactive ones.
     */
    auto cursor = std::stable_partition(
        std::begin(m_sinks), std::end(m_sinks), [](const auto& item) {
            return (item.template get<sink>().active());
        });

    /* Remove sinks from our workers */
    std::for_each(cursor, std::end(m_sinks), [&](auto& item) {
        remove_sink(m_client, item);
    });

    /* And erase from existence */
    m_sinks.erase(cursor, std::end(m_sinks));

    return (reply_ok{});
}

reply_msg server::handle_request(const request_get_analyzer& request)
{
    auto result = binary_find(std::begin(m_sinks),
                              std::end(m_sinks),
                              request.id,
                              sink_id_comparator{});

    if (result == std::end(m_sinks)) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_analyzers{};
    reply.analyzers.emplace_back(to_swagger(result->template get<sink>()));
    return (reply);
}

reply_msg server::handle_request(const request_delete_analyzer& request)
{
    auto result = binary_find(std::begin(m_sinks),
                              std::end(m_sinks),
                              request.id,
                              sink_id_comparator{});

    if (result != std::end(m_sinks) && !result->template get<sink>().active()) {
        /* Delete this analyzers result objects */
        erase_if(m_results, [&](const auto& pair) {
            return (pair.second->parent().id() == request.id);
        });

        /* Delete this analyzer */
        remove_sink(m_client, *result);
        m_sinks.erase(std::remove(result, std::next(result), *result),
                      std::end(m_sinks));
    }

    return (reply_ok{});
}

template <typename Map> static core::uuid get_unique_result_id(const Map& map)
{
    auto id = api::get_analyzer_result_id();
    while (map.count(id)) { id = api::get_analyzer_result_id(); }
    return (id);
}

reply_msg server::handle_request(const request_reset_analyzer& request)
{
    const auto found = binary_find(std::begin(m_sinks),
                                   std::end(m_sinks),
                                   request.id,
                                   sink_id_comparator{});

    if (found == std::end(m_sinks)) {
        return (to_error(error_type::NOT_FOUND));
    }

    if (!found->active()) { return (to_error(error_type::POSIX, EINVAL)); }

    auto& impl = found->template get<sink>();
    const auto item = m_results.emplace(get_unique_result_id(m_results),
                                        std::make_unique<sink_result>(impl));
    assert(item.second); /* sink_result inserted */

    const auto& id = item.first->first;
    const auto& result = item.first->second;

    impl.reset(result.get());

    auto reply = reply_analyzer_results{};
    reply.analyzer_results.emplace_back(to_swagger(id, *result));
    return (reply);
}

reply_msg server::handle_request(const request_start_analyzer& request)
{
    auto found = binary_find(std::begin(m_sinks),
                             std::end(m_sinks),
                             request.id,
                             sink_id_comparator{});

    if (found == std::end(m_sinks)) {
        return (to_error(error_type::NOT_FOUND));
    }

    if (found->active()) { return (to_error(error_type::POSIX, EINVAL)); }

    auto& impl = found->template get<sink>();
    auto item = m_results.emplace(get_unique_result_id(m_results),
                                  std::make_unique<sink_result>(impl));
    assert(item.second); /* sink_result inserted */

    auto& id = item.first->first;
    auto& result = item.first->second;

    impl.start(result.get());

    auto reply = reply_analyzer_results{};
    reply.analyzer_results.emplace_back(to_swagger(id, *result));
    return (reply);
}

reply_msg server::handle_request(const request_stop_analyzer& request)
{
    if (auto found = binary_find(std::begin(m_sinks),
                                 std::end(m_sinks),
                                 request.id,
                                 sink_id_comparator{});
        found != std::end(m_sinks)) {
        auto& impl = found->template get<sink>();
        impl.stop();
    }

    return (reply_ok{});
}

reply_msg server::handle_request(const request_bulk_create_analyzers& request)
{
    auto bulk_reply = reply_analyzers{};
    auto bulk_errors = std::vector<reply_error>{};

    std::for_each(
        std::begin(request.analyzers),
        std::end(request.analyzers),
        [&](const auto& analyzer) {
            auto api_reply = handle_request(request_create_analyzer{
                std::make_unique<analyzer_type>(*analyzer)});
            if (auto reply = std::get_if<reply_analyzers>(&api_reply)) {
                assert(reply->analyzers.size() == 1);
                std::move(std::begin(reply->analyzers),
                          std::end(reply->analyzers),
                          std::back_inserter(bulk_reply.analyzers));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        /* Roll back! */
        std::for_each(std::begin(bulk_reply.analyzers),
                      std::end(bulk_reply.analyzers),
                      [&](const auto& analyzer) {
                          handle_request(
                              request_delete_analyzer{analyzer->getId()});
                      });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_bulk_delete_analyzers& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_delete_analyzer{*id});
        });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_bulk_start_analyzers& request)
{
    auto bulk_reply = reply_analyzer_results{};
    auto bulk_errors = std::vector<reply_error>{};

    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            auto api_reply = handle_request(request_start_analyzer{*id});
            if (auto reply = std::get_if<reply_analyzer_results>(&api_reply)) {
                assert(reply->analyzer_results.size() == 1);
                std::move(std::begin(reply->analyzer_results),
                          std::end(reply->analyzer_results),
                          std::back_inserter(bulk_reply.analyzer_results));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        /* Undo what we have done */
        std::for_each(
            std::begin(bulk_reply.analyzer_results),
            std::end(bulk_reply.analyzer_results),
            [&](const auto& result) {
                handle_request(request_stop_analyzer{result->getAnalyzerId()});
                handle_request(request_delete_analyzer_result{result->getId()});
            });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_bulk_stop_analyzers& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_stop_analyzer{*id});
        });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_list_analyzer_results& request)
{
    auto reply = reply_analyzer_results{};

    auto compare = std::function<bool(const result_map::value_type& pair)>{};
    if (!request.filter) {
        compare = [](const auto&) { return (true); };
    } else {
        auto& filter = *request.filter;
        compare = [&](const auto& item) {
            if (filter.count(filter_key_type::analyzer_id)
                && filter[filter_key_type::analyzer_id]
                       != item.second->parent().id()) {
                return (false);
            }

            if (filter.count(filter_key_type::source_id)
                && filter[filter_key_type::source_id]
                       != item.second->parent().source()) {
                return (false);
            }

            return (true);
        };
    }

    assert(compare);

    transform_if(std::begin(m_results),
                 std::end(m_results),
                 std::back_inserter(reply.analyzer_results),
                 compare,
                 [](const auto& item) {
                     return (to_swagger(item.first, *item.second));
                 });

    return (reply);
}

reply_msg server::handle_request(const request_delete_analyzer_results&)
{
    /* Delete all inactive results */
    erase_if(m_results, [](const auto& pair) {
        return (!pair.second->parent().active());
    });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_get_analyzer_result& request)
{
    auto id = to_uuid(request.id);
    if (!id) { return (to_error(error_type::NOT_FOUND)); }

    auto result = m_results.find(*id);
    if (result == std::end(m_results)) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_analyzer_results{};
    reply.analyzer_results.emplace_back(
        to_swagger(result->first, *result->second));
    return (reply);
}

reply_msg server::handle_request(const request_delete_analyzer_result& request)
{
    if (auto id = to_uuid(request.id); id.has_value()) {
        if (auto result = m_results.find(*id); result != std::end(m_results)) {
            if (!result->second->parent().active()) { m_results.erase(*id); }
        }
    }

    return (reply_ok{});
}

reply_msg server::handle_request(const request_list_rx_flows& request)
{
    auto compare = std::function<bool(const result_map::value_type& pair)>{};
    if (!request.filter) {
        compare = [](const auto&) { return (true); };
    } else {
        auto& filter = *request.filter;
        compare = [&](const auto& item) {
            if (filter.count(filter_key_type::analyzer_id)
                && filter[filter_key_type::analyzer_id]
                       != item.second->parent().id()) {
                return (false);
            }

            if (filter.count(filter_key_type::source_id)
                && filter[filter_key_type::source_id]
                       != item.second->parent().source()) {
                return (false);
            }

            return (true);
        };
    }

    assert(compare);

    auto reply = reply_rx_flows{};

    std::for_each(
        std::begin(m_results),
        std::end(m_results),
        [&](const auto& result_pair) {
            if (!compare(result_pair)) { return; }

            auto& shards = result_pair.second->flows();
            std::for_each(
                std::begin(shards), std::end(shards), [&](const auto& shard) {
                    uint16_t idx = 0;
                    auto guard = utils::recycle::guard(shard.first,
                                                       api::result_reader_id);
                    std::transform(
                        std::begin(shard.second),
                        std::end(shard.second),
                        std::back_inserter(reply.flows),
                        [&](const auto& pair) {
                            return (to_swagger(rx_flow_id(result_pair.first,
                                                          idx,
                                                          pair.first.first,
                                                          pair.first.second),
                                               result_pair.first,
                                               pair.second));
                        });
                });
        });

    return (reply);
}

reply_msg server::handle_request(const request_get_rx_flow& request)
{
    auto id = to_uuid(request.id);
    if (!id) { return (to_error(error_type::NOT_FOUND)); }

    auto [min_id, shard_idx, hash, stream_id] = rx_flow_tuple(*id);

    auto it = m_results.lower_bound(min_id);
    if (it == std::end(m_results)) { return (to_error(error_type::NOT_FOUND)); }

    const auto& result = it->second;
    if (shard_idx >= result->flows().size()) {
        return (to_error(error_type::NOT_FOUND));
    }

    const auto& shard = result->flows()[shard_idx];
    auto guard = utils::recycle::guard(shard.first, api::result_reader_id);
    auto counters = shard.second.find(hash, stream_id);
    if (!counters) { return (to_error(error_type::NOT_FOUND)); }

    auto reply = reply_rx_flows{};
    reply.flows.emplace_back(to_swagger(*id, it->first, *counters));
    return (reply);
}

} // namespace openperf::packet::analyzer::api
