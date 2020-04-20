#include <zmq.h>

#include "packet/capture/server.hpp"
#include "swagger/v1/model/PacketCapture.h"
#include "swagger/v1/model/PacketCaptureResult.h"
#include "utils/overloaded_visitor.hpp"

namespace openperf::packet::capture::api {

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
    bool operator()(const packetio::packets::generic_sink& left,
                    std::string_view right)
    {
        return (left.id() < right);
    }

    bool operator()(std::string_view left,
                    const packetio::packets::generic_sink& right)
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
                           [](const request_list_captures&) {
                               return (std::string("list captures"));
                           },
                           [](const request_create_capture&) {
                               return (std::string("create capture"));
                           },
                           [](const request_delete_captures&) {
                               return (std::string("delete captures"));
                           },
                           [](const request_get_capture& request) {
                               return ("get capture " + request.id);
                           },
                           [](const request_delete_capture& request) {
                               return ("delete capture " + request.id);
                           },
                           [](const request_start_capture& request) {
                               return ("start capture " + request.id);
                           },
                           [](const request_stop_capture& request) {
                               return ("stop capture " + request.id);
                           },
                           [](const request_list_capture_results&) {
                               return (std::string("list capture results"));
                           },
                           [](const request_delete_capture_results&) {
                               return (std::string("delete capture results"));
                           },
                           [](const request_get_capture_result& request) {
                               return ("get capture result " + request.id);
                           },
                           [](const request_delete_capture_result& request) {
                               return ("delete capture result " + request.id);
                           },
                           [](const request_create_capture_transfer&) {
                               return (std::string("create capture transfer"));
                           },
                           [](const request_delete_capture_transfer& request) {
                               return (std::string("delete capture transfer "
                                                   + request.id));
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

reply_msg server::handle_request(const request_list_captures& request)
{
    auto reply = reply_captures{};

    if (request.filter && request.filter->count(filter_key_type::source_id)) {
        auto& source = (*request.filter)[filter_key_type::source_id];
        transform_if(std::begin(m_sinks),
                     std::end(m_sinks),
                     std::back_inserter(reply.captures),
                     [&](const auto& item) {
                         return (item.template get<sink>().source() == source);
                     },
                     [](const auto& item) {
                         return (to_swagger(item.template get<sink>()));
                     });
    } else {
        /* return all captures */
        std::transform(std::begin(m_sinks),
                       std::end(m_sinks),
                       std::back_inserter(reply.captures),
                       [&](const auto& item) {
                           return (to_swagger(item.template get<sink>()));
                       });
    }

    return (reply);
}

reply_msg server::handle_request(const request_create_capture& request)
{
    auto config = sink_config{.source = request.capture->getSourceId()};

    auto user_config = request.capture->getConfig();
    assert(user_config);

    if (!request.capture->getId().empty()) {
        config.id = request.capture->getId();
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
    auto success = m_client.add_sink(config.source, item);
    if (!success) {
        /*
         * Luckily, we failed adding the last item in the vector,
         * so just delete it.
         */
        m_sinks.erase(std::prev(std::end(m_sinks)));
        return (to_error(error_type::POSIX, success.error()));
    }

    /* Otherwise, sort our sinks and return */
    std::sort(std::begin(m_sinks),
              std::end(m_sinks),
              [](const auto& left, const auto& right) {
                  return (left.id() < right.id());
              });

    auto reply = reply_captures{};
    reply.captures.emplace_back(to_swagger(item.template get<sink>()));
    return (reply);
}

static void remove_sink(packetio::internal::api::client& client,
                        packetio::packets::generic_sink& to_add)
{
    if (auto success =
            client.del_sink(to_add.template get<sink>().source(), to_add);
        !success) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to remove capture %s from packetio workers!\n",
               to_add.id().c_str());
    }
}

reply_msg server::handle_request(const request_delete_captures&)
{
    /*
     * Sort captures into active and inactive ones.  We want to delete
     * all of the inactive ones.
     */
    auto cursor = std::stable_partition(
        std::begin(m_sinks), std::end(m_sinks), [](const auto& item) {
            return (item.template get<sink>().active());
        });

    /* Remove sinks from our workers */
    std::for_each(cursor, std::end(m_sinks), [&](auto& item) {
        remove_sink(m_client, item);
        // Remove all results for this sink
        erase_if(m_results, [&](const auto& pair) {
            return (&pair.second->parent == &item.template get<sink>());
        });
    });

    /* And erase from existence */
    m_sinks.erase(cursor, std::end(m_sinks));

    return (reply_ok{});
}

reply_msg server::handle_request(const request_get_capture& request)
{
    auto result = binary_find(std::begin(m_sinks),
                              std::end(m_sinks),
                              request.id,
                              sink_id_comparator{});

    if (result == std::end(m_sinks)) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_captures{};
    reply.captures.emplace_back(to_swagger(result->template get<sink>()));
    return (reply);
}

reply_msg server::handle_request(const request_delete_capture& request)
{
    auto result = binary_find(std::begin(m_sinks),
                              std::end(m_sinks),
                              request.id,
                              sink_id_comparator{});

    if (result != std::end(m_sinks) && !result->template get<sink>().active()) {
        // Remove sink from worker
        remove_sink(m_client, *result);
        // Remove all results for this sink
        erase_if(m_results, [&](const auto& pair) {
            return (&pair.second->parent == &result->template get<sink>());
        });
        // Delete this capture
        m_sinks.erase(std::remove(result, std::next(result), *result),
                      std::end(m_sinks));
    }

    return (reply_ok{});
}

reply_msg server::handle_request(const request_start_capture& request)
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
    auto item = m_results.emplace(core::uuid::random(),
                                  std::make_unique<sink_result>(impl));
    assert(item.second); /* sink_result inserted */

    auto& id = item.first->first;
    auto& result = item.first->second;

    // Create capture buffers and associate with result object
    auto idx = 0;
    std::generate_n(
        std::back_inserter(result->buffers), impl.worker_count(), [&]() {
            return std::make_unique<capture_buffer_file>(
                openperf::core::to_string(id) + "-" + std::to_string(++idx)
                    + ".pcapng",
                false);
        });

    impl.start(result.get());

    auto reply = reply_capture_results{};
    reply.capture_results.emplace_back(to_swagger(id, *result));
    return (reply);
}

reply_msg server::handle_request(const request_stop_capture& request)
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

reply_msg server::handle_request(const request_list_capture_results& request)
{
    auto reply = reply_capture_results{};

    auto compare = std::function<bool(const result_map::value_type& pair)>{};
    if (!request.filter) {
        compare = [](const auto&) { return (true); };
    } else {
        auto& filter = *request.filter;
        compare = [&](const auto& item) {
            if (filter.count(filter_key_type::capture_id)
                && filter[filter_key_type::capture_id]
                       != item.second->parent.id()) {
                return (false);
            }

            if (filter.count(filter_key_type::source_id)
                && filter[filter_key_type::source_id]
                       != item.second->parent.source()) {
                return (false);
            }

            return (true);
        };
    }

    assert(compare);

    transform_if(std::begin(m_results),
                 std::end(m_results),
                 std::back_inserter(reply.capture_results),
                 compare,
                 [](auto& item) {
                     item.second->update_stats();
                     return (to_swagger(item.first, *item.second));
                 });

    return (reply);
}

reply_msg server::handle_request(const request_delete_capture_results&)
{
    /* Delete all inactive results */
    erase_if(m_results,
             [](const auto& pair) { return (!pair.second->parent.active()); });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_get_capture_result& request)
{
    auto id = to_uuid(request.id);
    if (!id) { return (to_error(error_type::NOT_FOUND)); }

    auto result = m_results.find(*id);
    if (result == std::end(m_results)) {
        return (to_error(error_type::NOT_FOUND));
    }
    result->second->update_stats();

    auto reply = reply_capture_results{};
    reply.capture_results.emplace_back(
        to_swagger(result->first, *result->second));
    return (reply);
}

reply_msg server::handle_request(const request_delete_capture_result& request)
{
    if (auto id = to_uuid(request.id); id.has_value()) {
        if (auto item = m_results.find(*id); item != std::end(m_results)) {
            // FIXME: Should only allow delete if no transfer in progress
            auto& result = item->second;
            if (!result->parent.active()) { m_results.erase(*id); }
        }
    }

    return (reply_ok{});
}

reply_msg server::handle_request(request_create_capture_transfer& request)
{
    auto id = to_uuid(request.id);
    if (!id) { return (to_error(error_type::NOT_FOUND)); }

    auto item = m_results.find(*id);
    if (item == std::end(m_results)) {
        return (to_error(error_type::NOT_FOUND));
    }
    auto& result = item->second;

    if (result->transfer) {
        // Only support 1 transfer per capture results
        return (to_error(error_type::POSIX, EEXIST));
    }

    result->transfer = request.transfer;
    if (result->buffers.size() == 1) {
        auto reader = std::unique_ptr<capture_buffer_reader>(
            new capture_buffer_file_reader(*(result->buffers[0])));
        result->transfer->set_reader(reader);
    } else {
        std::vector<std::unique_ptr<capture_buffer_reader>> readers;
        std::transform(result->buffers.begin(),
                       result->buffers.end(),
                       std::back_inserter(readers),
                       [&](auto& buffer) {
                           return std::make_unique<capture_buffer_file_reader>(
                               *buffer);
                       });
        auto reader = std::unique_ptr<capture_buffer_reader>(
            new multi_capture_buffer_reader(std::move(readers)));
        result->transfer->set_reader(reader);
    }

    return (reply_ok{});
}

reply_msg server::handle_request(const request_delete_capture_transfer& request)
{
    if (auto id = to_uuid(request.id); id.has_value()) {
        if (auto item = m_results.find(*id); item != std::end(m_results)) {
            auto& result = item->second;
            result->transfer.reset();
        }
    }

    return (reply_ok{});
}

} // namespace openperf::packet::capture::api
