#include <zmq.h>

#include "packet/generator/server.hpp"
#include "message/serialized_message.hpp"
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
    bool operator()(const server::source_item& left, std::string_view right)
    {
        return (left.first.id() < right);
    }

    bool operator()(std::string_view left, const server::source_item& right)
    {
        return (left < right.first.id());
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
                           [](const request_bulk_create_generators&) {
                               return (std::string("bulk create generators"));
                           },
                           [](const request_bulk_delete_generators&) {
                               return (std::string("bulk delete generators"));
                           },
                           [](const request_bulk_start_generators&) {
                               return (std::string("bulk start generators"));
                           },
                           [](const request_bulk_stop_generators&) {
                               return (std::string("bulk stop generators"));
                           },
                           [](const request_toggle_generator& request) {
                               return ("toggle generator: " + request.ids->first
                                       + " --> " + request.ids->second);
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
                return (item.first.template get<source>().target() == target);
            },
            [](const auto& item) {
                return (to_swagger(item.first.template get<source>()));
            });
    } else {
        /* return all generators */
        std::transform(std::begin(m_sources),
                       std::end(m_sources),
                       std::back_inserter(reply.generators),
                       [&](const auto& item) {
                           return (
                               to_swagger(item.first.template get<source>()));
                       });
    }

    return (reply);
}

static tl::expected<void, api::reply_error>
add_source(packetio::internal::api::client& client, server::source_item& to_add)
{
    if (to_add.second == server::source_state::active) { return {}; }

    if (auto success = client.add_source(
            to_add.first.template get<source>().target(), to_add.first);
        !success) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to add generator %s to packetio workers!\n",
               to_add.first.id().c_str());
        return (
            tl::make_unexpected(to_error(error_type::POSIX, success.error())));
    }

    to_add.second = server::source_state::active;

    return {};
}

static void remove_source(packetio::internal::api::client& client,
                          server::source_item& to_del)
{
    if (to_del.second != server::source_state::active) { return; }

    if (auto success = client.del_source(
            to_del.first.template get<source>().target(), to_del.first);
        !success) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to remove generator %s from packetio workers!\n",
               to_del.first.id().c_str());
    }

    to_del.second = server::source_state::idle;
}

static tl::expected<void, api::reply_error>
swap_source(packetio::internal::api::client& client,
            server::source_item& to_del,
            server::source_item& to_add,
            packetio::workers::source_swap_function&& action)
{
    assert(to_del.second == server::source_state::active);
    assert(to_add.second == server::source_state::idle);

    if (auto success =
            client.swap_source(to_del.first.template get<source>().target(),
                               to_del.first,
                               to_add.first,
                               std::forward<decltype(action)>(action));
        !success) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to swap generator %s for generator %s in packetio "
               "workers!\n",
               to_del.first.id().c_str(),
               to_add.first.id().c_str());
        return (
            tl::make_unexpected(to_error(error_type::POSIX, success.error())));
    }

    to_del.second = server::source_state::idle;
    to_add.second = server::source_state::active;

    return {};
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

    auto& item =
        m_sources.emplace_back(source(std::move(config)), source_state::idle);

    /* Grab a reference before we invalidate the iterator */
    const auto& impl = item.first.template get<source>();

    /* Success; sort sources and return */
    std::sort(std::begin(m_sources),
              std::end(m_sources),
              [](const auto& left, const auto& right) {
                  return (left.first.id() < right.first.id());
              });

    auto reply = reply_generators{};
    reply.generators.emplace_back(to_swagger(impl));
    return (reply);
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
            return (item.first.template get<source>().active());
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
    reply.generators.emplace_back(
        to_swagger(result->first.template get<source>()));
    return (reply);
}

reply_msg server::handle_request(const request_delete_generator& request)
{
    auto result = binary_find(std::begin(m_sources),
                              std::end(m_sources),
                              request.id,
                              source_id_comparator{});

    if (result != std::end(m_sources)
        && !result->first.template get<source>().active()) {
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

    if (found->first.active()) { return (to_error(error_type::POSIX, EINVAL)); }

    auto& impl = found->first.template get<source>();

    auto item = m_results.emplace(get_unique_result_id(m_results),
                                  std::make_unique<source_result>(impl));
    assert(item.second); /* source_result inserted */

    auto& id = item.first->first;
    auto& result = item.first->second;

    impl.start(result.get());

    if (auto success = add_source(m_client, *found); !success) {
        impl.stop();
        m_results.erase(id);
        return (success.error());
    }

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
        auto& impl = found->first.template get<source>();
        impl.stop();
    }

    return (reply_ok{});
}

reply_msg server::handle_request(const request_bulk_create_generators& request)
{
    auto bulk_reply = reply_generators{};
    auto bulk_errors = std::vector<reply_error>{};

    /* XXX: making a copy due to a bad choice in function signature */
    std::for_each(
        std::begin(request.generators),
        std::end(request.generators),
        [&](const auto& generator) {
            auto api_reply = handle_request(request_create_generator{
                std::make_unique<generator_type>(*generator)});
            if (auto reply = std::get_if<reply_generators>(&api_reply)) {
                assert(reply->generators.size() == 1);
                std::move(std::begin(reply->generators),
                          std::end(reply->generators),
                          std::back_inserter(bulk_reply.generators));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        /* Roll back */
        std::for_each(std::begin(bulk_reply.generators),
                      std::end(bulk_reply.generators),
                      [&](const auto& generator) {
                          handle_request(
                              request_delete_generator{generator->getId()});
                      });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_bulk_delete_generators& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_delete_generator{*id});
        });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_bulk_start_generators& request)
{
    auto bulk_reply = reply_generator_results{};
    auto bulk_errors = std::vector<reply_error>{};

    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            auto api_reply = handle_request(request_start_generator{*id});
            if (auto reply = std::get_if<reply_generator_results>(&api_reply)) {
                assert(reply->generator_results.size() == 1);
                std::move(std::begin(reply->generator_results),
                          std::end(reply->generator_results),
                          std::back_inserter(bulk_reply.generator_results));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        /* Undo! */
        std::for_each(std::begin(bulk_reply.generator_results),
                      std::end(bulk_reply.generator_results),
                      [&](const auto& result) {
                          handle_request(
                              request_stop_generator{result->getGeneratorId()});
                          handle_request(
                              request_delete_generator_result{result->getId()});
                      });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_bulk_stop_generators& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_stop_generator{*id});
        });

    return (reply_ok{});
}

static void do_source_swap(const packetio::packet::generic_source& outgoing,
                           packetio::packet::generic_source& incoming,
                           server::result_value_type* result)
{
    using sig_config_type = std::optional<traffic::signature_config>;

    auto& out_source = outgoing.template get<source>();
    auto& in_source = incoming.template get<source>();

    auto out_results = out_source.stop();

    /* Generate offset map for signature flows */
    auto sig_offsets = std::map<uint32_t, traffic::stat_t>{};
    const auto& out_sequence = out_source.sequence();
    std::for_each(std::begin(out_sequence),
                  std::end(out_sequence),
                  [&](const auto& tuple) {
                      const auto& sig_config = std::get<sig_config_type>(tuple);
                      if (sig_config) {
                          sig_offsets[sig_config->stream_id] =
                              (*out_results)[std::get<0>(tuple)].packet;
                      }
                  });

    in_source.start(result, sig_offsets);
}

reply_msg server::handle_request(const request_toggle_generator& request)
{
    auto out_found = binary_find(std::begin(m_sources),
                                 std::end(m_sources),
                                 request.ids->first,
                                 source_id_comparator{});
    auto in_found = binary_find(std::begin(m_sources),
                                std::end(m_sources),
                                request.ids->second,
                                source_id_comparator{});

    if (out_found == std::end(m_sources) || in_found == std::end(m_sources)) {
        return (to_error(error_type::NOT_FOUND));
    }

    if (out_found->second == source_state::idle
        || in_found->second == source_state::active) {
        return (to_error(error_type::POSIX, EINVAL));
    }

    auto& in_impl = in_found->first.template get<source>();
    auto item = m_results.emplace(get_unique_result_id(m_results),
                                  std::make_unique<source_result>(in_impl));
    assert(item.second);

    auto& id = item.first->first;
    auto& result = item.first->second;

    /* Now; do the swap */
    auto success =
        swap_source(m_client,
                    *out_found,
                    *in_found,
                    [&](const auto& outgoing, auto& incoming) {
                        do_source_swap(outgoing, incoming, result.get());
                    });
    if (!success) {
        in_impl.stop();
        m_results.erase(id);
        return (success.error());
    }

    /* Return the new result to the user */
    auto reply = reply_generator_results{};
    reply.generator_results.emplace_back(to_swagger(id, *result));
    return (reply);
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
            if (!result->second->active()) { m_results.erase(*id); }
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

                      const auto& flows = result_pair.second->flows();
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
    if (flow_idx >= result->flows().size()) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_tx_flows{};
    reply.flows.emplace_back(to_swagger(*id, it->first, *result, flow_idx));
    return (reply);
}

} // namespace openperf::packet::generator::api
