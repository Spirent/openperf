#include <cinttypes>
#include <zmq.h>
#include <sys/eventfd.h>

#include "message/serialized_message.hpp"
#include "packet/bpf/bpf.hpp"
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

template <typename ContainerSrc,
          typename ContainerDst,
          typename Condition,
          typename Move>
static void
move_if(ContainerSrc& c, ContainerDst& d, Condition&& op, Move&& move_op)
{
    auto cursor = std::begin(c);
    while (cursor != std::end(c)) {
        if (op(*cursor)) {
            move_op(d, *cursor);
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
                           [](const request_bulk_create_captures& request) {
                               return (std::string("bulk create captures"));
                           },
                           [](const request_bulk_delete_captures& request) {
                               return (std::string("bulk delete captures"));
                           },
                           [](const request_bulk_start_captures& request) {
                               return (std::string("bulk start captures"));
                           },
                           [](const request_bulk_stop_captures& request) {
                               return (std::string("bulk stop captures"));
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

server::event_trigger::event_trigger(
    std::function<void(const event_trigger&)>&& cb)
    : callback(cb)
{
    fd = eventfd(0, EFD_NONBLOCK);
    if (fd < 0) { throw std::runtime_error("Unable to create eventfd"); }
}

server::event_trigger::~event_trigger() { close(fd); }

int server::event_trigger::trigger()
{
    uint64_t val = 1;

    OP_LOG(OP_LOG_DEBUG, "Event trigger triggered fd %d", fd);

    if (write(fd, &val, sizeof(val)) != sizeof(val)) {
        OP_LOG(OP_LOG_ERROR, "Unable to write to event trigger fd %d", fd);
        return -1;
    }
    return 0;
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

bool server::has_active_transfer(const sink& sink) const
{
    // Check all results which have the sink as the parent
    if (const auto found =
            std::find_if(m_results.begin(),
                         m_results.end(),
                         [&](const auto& pair) {
                             const auto& result = pair.second;
                             if (&result->parent == &sink) {
                                 return result->has_active_transfer();
                             }
                             return false;
                         });
        found != m_results.end()) {
        return true;
    }

    // It is possible that a result may be deleted with an active
    // transfer, so need to check for transfers in the trash.
    if (const auto found = m_trash.find(sink.id()); found != m_trash.end()) {
        const auto& bucket = found->second;
        const auto& results = bucket->results;
        const auto found_result =
            std::find_if(results.begin(), results.end(), [](auto& result) {
                return result->has_active_transfer();
            });
        return found_result != results.end();
    }

    return false;
}

reply_msg server::handle_request(const request_list_captures& request)
{
    auto reply = reply_captures{};

    if (request.filter && request.filter->count(filter_key_type::source_id)) {
        auto& source = (*request.filter)[filter_key_type::source_id];
        transform_if(
            std::begin(m_sinks),
            std::end(m_sinks),
            std::back_inserter(reply.captures),
            [&](const auto& item) {
                return (item.template get<sink>().source() == source);
            },
            [&](const auto& item) {
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
    auto config = sink_config{.source = request.capture->getSourceId(),
                              .max_packet_size = UINT32_MAX,
                              .duration = {}};

    auto user_config = request.capture->getConfig();
    assert(user_config);
    config.capture_mode = capture_mode_from_string(user_config->getMode());
    config.buffer_wrap = user_config->isBufferWrap();
    config.buffer_size = user_config->getBufferSize();
    if (user_config->packetSizeIsSet())
        config.max_packet_size = user_config->getPacketSize();
    if (user_config->durationIsSet())
        config.duration = std::chrono::duration<uint64_t, std::milli>(
            user_config->getDuration());
    if (user_config->filterIsSet()) {
        config.filter = user_config->getFilter();
    }
    if (user_config->startTriggerIsSet()) {
        config.start_trigger = user_config->getStartTrigger();
    }
    if (user_config->stopTriggerIsSet()) {
        config.stop_trigger = user_config->getStopTrigger();
    }
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

    /* Grab a reference before we invalidate the iterator */
    const auto& impl = item.template get<sink>();

    /* Otherwise, sort our sinks and return */
    std::sort(std::begin(m_sinks),
              std::end(m_sinks),
              [](const auto& left, const auto& right) {
                  return (left.id() < right.id());
              });

    auto reply = reply_captures{};
    reply.captures.emplace_back(to_swagger(impl));
    return (reply);
}

static void remove_sink(packetio::internal::api::client& client,
                        packetio::packet::generic_sink& to_add)
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
        std::begin(m_sinks), std::end(m_sinks), [&](const auto& item) {
            auto& sink_ref = item.template get<sink>();
            return (sink_ref.active());
        });

    /* Remove sinks from our workers */
    std::for_each(cursor, std::end(m_sinks), [&](auto& item) {
        remove_sink(m_client, item);
    });

    /* Sort deleted items so deferred deletion items are last */
    auto defer_cursor =
        std::stable_partition(cursor, std::end(m_sinks), [&](const auto& item) {
            auto& sink_ref = item.template get<sink>();
            return !has_active_transfer(sink_ref);
        });
    /* Move deferred deletion items to delete list */
    std::for_each(
        defer_cursor, std::end(m_sinks), [&](auto& item) { add_trash(item); });
    /* Erase deferred delete items */
    m_sinks.erase(defer_cursor, std::end(m_sinks));

    /* Remove all results for this sink */
    std::for_each(cursor, std::end(m_sinks), [&](auto& item) {
        auto& sink_ref = item.template get<sink>();
        erase_if(m_results, [&](const auto& pair) {
            return (&pair.second->parent == &sink_ref);
        });
    });

    /* And erase from existence */
    m_sinks.erase(cursor, std::end(m_sinks));

    return (reply_ok{});
}

reply_msg server::handle_request(const request_get_capture& request)
{
    auto found = binary_find(std::begin(m_sinks),
                             std::end(m_sinks),
                             request.id,
                             sink_id_comparator{});

    if (found == std::end(m_sinks)) {
        return (to_error(error_type::NOT_FOUND));
    }

    auto reply = reply_captures{};
    reply.captures.emplace_back(to_swagger(found->template get<sink>()));
    return (reply);
}

reply_msg server::handle_request(const request_delete_capture& request)
{
    auto found = binary_find(std::begin(m_sinks),
                             std::end(m_sinks),
                             request.id,
                             sink_id_comparator{});

    if (found != std::end(m_sinks) && !found->template get<sink>().active()) {
        // Remove sink from worker
        remove_sink(m_client, *found);

        auto& sink_ref = found->template get<sink>();

        if (has_active_transfer(sink_ref)) {
            add_trash(*found);
        } else {
            // Remove all results for this sink
            erase_if(m_results, [&](const auto& pair) {
                return (&pair.second->parent == &sink_ref);
            });
        }

        // Delete this capture
        m_sinks.erase(std::remove(found, std::next(found), *found),
                      std::end(m_sinks));
    }

    return (reply_ok{});
}

std::unique_ptr<capture_buffer>
create_capture_buffer([[maybe_unused]] const core::uuid& id,
                      [[maybe_unused]] const sink& sink,
                      [[maybe_unused]] int worker)
{
    auto& config = sink.get_config();

    switch (config.capture_mode) {
    case capture_mode::BUFFER: {
        if (config.buffer_wrap) {
            return std::unique_ptr<capture_buffer>(new capture_buffer_mem_wrap(
                config.buffer_size, config.max_packet_size));
        }
        return std::unique_ptr<capture_buffer>(
            new capture_buffer_mem(config.buffer_size, config.max_packet_size));
    }
    case capture_mode::LIVE: {
        // TODO: Add support for live capture mode
        OP_LOG(OP_LOG_ERROR, "Live capture mode is not supported yet.");
        throw std::bad_alloc();
    }
    case capture_mode::FILE: {
        auto filename = openperf::core::to_string(id) + "-"
                        + std::to_string(worker) + ".pcapng";
        return std::unique_ptr<capture_buffer>(
            new capture_buffer_file(filename,
                                    capture_buffer_file::keep_file::disabled,
                                    config.max_packet_size));
    }
    }
}

static int handle_capture_stop(const struct op_event_data* data, void* arg)
{
    auto srv = reinterpret_cast<server*>(arg);
    assert(srv);
    return srv->handle_capture_stop_timer(data->timeout_id);
}

int server::handle_capture_stop_timer(uint32_t timeout_id)
{
    OP_LOG(OP_LOG_DEBUG, "Processing timer id %#" PRIx32, timeout_id);

    auto found =
        std::find_if(m_results.begin(), m_results.end(), [&](const auto& pair) {
            auto& result = pair.second;
            return (result->timeout_id == timeout_id);
        });
    if (found == m_results.end()) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to find capture result matching timer id %#" PRIx32,
               timeout_id);
        return -1;
    }
    auto& id = found->first;
    auto& result = found->second;
    if (result->state != capture_state::STOPPED) {
        OP_LOG(OP_LOG_DEBUG,
               "Capture %s stopped due to duration timeout",
               to_string(id).c_str());
        // FIXME: Need to do some cleanup so we don't need a const_cast
        const_cast<sink&>(result->parent).stop();
        result->timeout_id = 0;
        return -1;
    }

    return 0;
}

void server::add_capture_start_trigger(const core::uuid& id,
                                       result_value_type& result)
{
    auto event = std::make_shared<event_trigger>([srv = this, id](auto evt) {
        // This code will be called from the server thread
        auto found = srv->m_results.find(id);
        if (found == srv->m_results.end()) {
            // Result was already deleted so just remove the trigger
            OP_LOG(OP_LOG_DEBUG,
                   "Capture result %s state changed but result not found",
                   to_string(id).c_str());
            srv->remove_event_trigger(evt.fd);
            return;
        }
        auto& result = found->second;
        capture_state state = result->state;
        OP_LOG(OP_LOG_DEBUG,
               "Capture %s result %s state changed to %d",
               result->parent.id().c_str(),
               to_string(id).c_str(),
               (int)state);
        if (state == capture_state::STARTED) {
            auto& config = result->parent.get_config();
            if (config.duration.count())
                srv->add_capture_stop_timer(*result, config.duration);
        } else if (state == capture_state::STOPPED) {
            // Worker shouldn't touch result in stopped state
            // so it is safe to remove the callback
            result->state_changed_callback = nullptr;
            srv->remove_event_trigger(evt.fd);
        }
    });

    result.state_changed_callback = [event]([[maybe_unused]] auto& result) {
        // State change callback may be called from either server thread
        // or worker thread so need to trigger event to run in server thread
        OP_LOG(OP_LOG_DEBUG,
               "Capture %s state changed",
               result.parent.id().c_str());
        event->trigger();
    };
    add_event_trigger(event);
}

void server::add_capture_stop_timer(
    result_value_type& result,
    std::chrono::duration<uint64_t, std::nano> duration)
{
    auto callbacks = op_event_callbacks{.on_timeout = handle_capture_stop};
    auto timeout = duration.count();
    m_loop.add(timeout, &callbacks, this, &result.timeout_id);
    OP_LOG(OP_LOG_DEBUG,
           "Added capture duration timer id %#" PRIx32 " timeout %" PRId64,
           result.timeout_id,
           timeout);
}

reply_msg server::handle_request(const request_start_capture& request)
{
    auto found = binary_find(std::begin(m_sinks),
                             std::end(m_sinks),
                             request.id,
                             sink_id_comparator{});

    if (found == std::end(m_sinks)) {
        OP_LOG(OP_LOG_ERROR,
               "Capture start failed.  Unable to find capture %s.",
               request.id.c_str());
        return (to_error(error_type::NOT_FOUND));
    }

    if (found->active()) {
        OP_LOG(OP_LOG_ERROR,
               "Capture start failed.  Capture %s is already running.",
               request.id.c_str());
        return (to_error(error_type::POSIX, EINVAL));
    }

    auto& impl = found->template get<sink>();
    auto item = m_results.emplace(core::uuid::random(),
                                  std::make_unique<sink_result>(impl));
    assert(item.second); /* sink_result inserted */

    auto& id = item.first->first;
    auto& result = item.first->second;

    try {
        // Create capture buffers for each worker and add to result object
        for (size_t worker = 0, n = impl.worker_count(); worker < n; ++worker) {
            result->buffers.emplace_back(
                create_capture_buffer(id, impl, worker));
        }
    } catch (const std::bad_alloc& e) {
        OP_LOG(OP_LOG_ERROR, "Failed allocating capture buffer.  %s", e.what());
        auto count = m_results.erase(id);
        if (count == 0) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed removing capture result %s",
                   to_string(id).c_str());
        }
        return (to_error(error_type::POSIX, ENOMEM));
    }

    // If capture duration is being used then need to add a callback to
    // be called when capture is started to handle the duration timeout.
    auto& config = impl.get_config();
    if (config.duration.count()) { add_capture_start_trigger(id, *result); }

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

        if (auto result = impl.get_result()) {
            if (result->timeout_id) {
                if (m_loop.disable(result->timeout_id) < 0) {
                    OP_LOG(OP_LOG_ERROR,
                           "Failed to disable capture duration timer %#" PRIx32,
                           result->timeout_id);
                }
                result->timeout_id = 0;
            }
        }
        impl.stop();
    }

    return (reply_ok{});
}

reply_msg server::handle_request(const request_bulk_create_captures& request)
{
    auto bulk_reply = reply_captures{};
    auto bulk_errors = std::vector<reply_error>{};

    std::for_each(
        std::begin(request.captures),
        std::end(request.captures),
        [&](const auto& capture) {
            auto api_reply = handle_request(request_create_capture{
                std::make_unique<capture_type>(*capture)});
            if (auto reply = std::get_if<reply_captures>(&api_reply)) {
                auto id = reply->captures[0]->getId();
                auto source = reply->captures[0]->getSourceId();
                assert(reply->captures.size() == 1);
                std::move(std::begin(reply->captures),
                          std::end(reply->captures),
                          std::back_inserter(bulk_reply.captures));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        /* Roll back! */
        std::for_each(std::begin(bulk_reply.captures),
                      std::end(bulk_reply.captures),
                      [&](const auto& capture) {
                          handle_request(
                              request_delete_capture{capture->getId()});
                      });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_bulk_delete_captures& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_delete_capture{*id});
        });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_bulk_start_captures& request)
{
    auto bulk_reply = reply_capture_results{};
    auto bulk_errors = std::vector<reply_error>{};

    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            auto api_reply = handle_request(request_start_capture{*id});
            if (auto reply = std::get_if<reply_capture_results>(&api_reply)) {
                assert(reply->capture_results.size() == 1);
                std::move(std::begin(reply->capture_results),
                          std::end(reply->capture_results),
                          std::back_inserter(bulk_reply.capture_results));
            } else {
                assert(std::holds_alternative<reply_error>(api_reply));
                bulk_errors.emplace_back(std::get<reply_error>(api_reply));
            }
        });

    if (!bulk_errors.empty()) {
        /* Undo what we have done */
        std::for_each(
            std::begin(bulk_reply.capture_results),
            std::end(bulk_reply.capture_results),
            [&](const auto& result) {
                handle_request(request_stop_capture{result->getCaptureId()});
                handle_request(request_delete_capture_result{result->getId()});
            });
        return (bulk_errors.front());
    }

    return (bulk_reply);
}

reply_msg server::handle_request(const request_bulk_stop_captures& request)
{
    std::for_each(
        std::begin(request.ids), std::end(request.ids), [&](const auto& id) {
            handle_request(request_stop_capture{*id});
        });

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

    transform_if(
        std::begin(m_results),
        std::end(m_results),
        std::back_inserter(reply.capture_results),
        compare,
        [](auto& item) { return (to_swagger(item.first, *item.second)); });

    return (reply);
}

reply_msg server::handle_request(const request_delete_capture_results&)
{
    /* Delete all inactive results */
    erase_if(m_results, [](const auto& pair) {
        auto& result = pair.second;
        if (result->transfer.get() && !result->transfer->is_done()) {
            return false;
        }
        return (!result->parent.active());
    });

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

    auto reply = reply_capture_results{};
    reply.capture_results.emplace_back(
        to_swagger(result->first, *result->second));
    return (reply);
}

reply_msg server::handle_request(const request_delete_capture_result& request)
{
    if (auto id = to_uuid(request.id); id.has_value()) {
        if (auto item = m_results.find(*id); item != std::end(m_results)) {
            auto& result = item->second;
            if (!result->parent.active()) {
                if (result->has_active_transfer()) {
                    add_trash(std::move(result));
                }
                m_results.erase(*id);
            }
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

    if (result->transfer && !result->transfer->is_done()) {
        // Only support 1 active transfer per capture result
        OP_LOG(OP_LOG_ERROR,
               "Capture pcap transfer request for id %s rejected.  "
               "Another transfer is still in progress.",
               request.id.c_str());
        return (to_error(error_type::POSIX, EEXIST));
    }

    result->transfer.reset(request.transfer);
    if (result->buffers.size() == 1) {
        auto reader = result->buffers[0]->create_reader();
        result->transfer->set_reader(reader);
    } else {
        std::vector<std::unique_ptr<capture_buffer_reader>> readers;
        std::transform(result->buffers.begin(),
                       result->buffers.end(),
                       std::back_inserter(readers),
                       [&](auto& buffer) { return buffer->create_reader(); });
        auto reader = std::unique_ptr<capture_buffer_reader>(
            new multi_capture_buffer_reader(std::move(readers)));
        result->transfer->set_reader(reader);
    }

    auto event = std::make_shared<event_trigger>([srv = this](const auto& evt) {
        srv->garbage_collect();
        srv->remove_event_trigger(evt.fd);
    });
    add_event_trigger(event);

    result->transfer->set_done_callback([event]() {
        // Transfer done callback is called from the transfer thread so need to
        // trigger event for server thread to do garbage collection
        OP_LOG(OP_LOG_DEBUG, "Scheduling capture garbage collection");
        event->trigger();
    });

    return (reply_ok{});
}

reply_msg server::handle_request(const request_delete_capture_transfer& request)
{
    if (auto id = to_uuid(request.id); id.has_value()) {
        if (auto item = m_results.find(*id); item != std::end(m_results)) {
            auto& result = item->second;

            if (result->has_active_transfer()) {
                OP_LOG(
                    OP_LOG_ERROR,
                    "Can not delete capture transfer when it is in progress.");
                return (to_error(error_type::POSIX, EBUSY));
            }
            result->transfer.reset();
        }
    }

    return (reply_ok{});
}

void server::add_trash(sink_value_type& item)
{
    auto& sink_ref = item.template get<sink>();
    auto& bucket = m_trash[item.id()];
    if (!bucket) { bucket = std::make_unique<trash_bucket>(); }
    OP_LOG(OP_LOG_DEBUG, "Added capture %s to trash.", sink_ref.id().c_str());
    bucket->sink = item;
    move_if(
        m_results,
        bucket->results,
        [&](const auto& pair) { return (&pair.second->parent == &sink_ref); },
        [&](auto& results, auto& pair) {
            results.emplace_back(std::move(pair.second));
        });
}

void server::add_trash(result_value_ptr&& result)
{
    auto id = result->parent.id();
    auto& bucket = m_trash[id];
    if (!bucket) { bucket = std::make_unique<trash_bucket>(); }
    OP_LOG(OP_LOG_DEBUG,
           "Added capture result for capture %s to trash",
           id.c_str());
    bucket->results.emplace_back(std::forward<result_value_ptr>(result));
}

int server::garbage_collect()
{
    OP_LOG(OP_LOG_DEBUG, "Starting capture garbage collection");

    // Delete items in the trash without results with active transfers
    erase_if(m_trash, [&](auto& pair) {
        auto& item = pair.second;
        auto found = std::find_if(
            item->results.begin(), item->results.end(), [](auto& result) {
                return result->has_active_transfer();
            });
        if (found == item->results.end()) {
            if (item->sink.has_value()) {
                OP_LOG(OP_LOG_DEBUG,
                       "Deleting %s capture and %zu results from garbage",
                       pair.first.c_str(),
                       item->results.size());
            } else {
                OP_LOG(OP_LOG_DEBUG,
                       "Deleting %s %zu results from garbage",
                       pair.first.c_str(),
                       item->results.size());
            }
            return true;
        }
        return false;
    });

    // Cleanup any completed transfers
    std::for_each(m_results.begin(), m_results.end(), [&](auto& pair) {
        auto& result = pair.second;
        if (result->transfer && result->transfer->is_done()) {
            OP_LOG(OP_LOG_DEBUG,
                   "Deleting capture transfer %s",
                   to_string(pair.first).c_str());
            result->transfer.reset();
        }
    });

    OP_LOG(OP_LOG_DEBUG, "Completed capture garbage collection");
    return -1;
}

static int handle_event_trigger_callback(const op_event_data* data, void* arg)
{
    auto s = reinterpret_cast<server*>(arg);
    return s->handle_event_trigger(data->fd);
}

void server::add_event_trigger(const std::shared_ptr<event_trigger>& trigger)
{
    OP_LOG(OP_LOG_DEBUG, "Event trigger add fd %d", trigger->fd);
    m_triggers[trigger->fd] = trigger;
    op_event_callbacks callbacks{.on_read = handle_event_trigger_callback};
    m_loop.add(trigger->fd, &callbacks, this);
}

void server::remove_event_trigger(int fd)
{
    OP_LOG(OP_LOG_DEBUG, "Event trigger remove fd %d", fd);
    auto found = m_triggers.find(fd);
    if (found == m_triggers.end()) return;
    m_loop.del(fd);
    found->second.reset();
    m_triggers.erase(found);
}

int server::handle_event_trigger(int fd)
{
    OP_LOG(OP_LOG_DEBUG, "Event trigger handler invoked for fd %d", fd);
    uint64_t val;

    read(fd, &val, sizeof(val));

    auto found = m_triggers.find(fd);
    if (found == m_triggers.end()) return -1;
    auto trigger = found->second;
    trigger->callback(*trigger);
    return 0;
}

} // namespace openperf::packet::capture::api
