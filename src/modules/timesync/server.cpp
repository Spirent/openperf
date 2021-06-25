#include <algorithm>
#include <cinttypes>
#include <cmath>

#include <arpa/inet.h>

#include "message/serialized_message.hpp"
#include "timesync/api.hpp"
#include "timesync/chrono.hpp"
#include "timesync/ntp.hpp"
#include "timesync/server.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::timesync::counter {
std::atomic<timecounter*> timecounter_now = nullptr;
}

namespace openperf::timesync::api {

static time_counter to_timecounter(const counter::timecounter& tc)
{
    auto counter = time_counter{.frequency = tc.frequency().count(),
                                .priority = tc.static_priority()};

    core::to_string(tc.id).copy(counter.id, id_max_length);
    tc.name().copy(counter.name, name_max_length);
    return (counter);
}

reply_msg server::handle_request(const request_time_counters& request)
{
    if (request.id) {
        auto item =
            std::find_if(std::begin(m_timecounters),
                         std::end(m_timecounters),
                         [&](const auto& tc) {
                             return (core::to_string(tc->id) == request.id);
                         });
        if (item != std::end(m_timecounters)) {
            auto reply = reply_time_counters{};
            reply.counters.emplace_back(to_timecounter(*(item->get())));
            return (reply);
        } else {
            return (to_error(error_type::NOT_FOUND));
        }
    }

    auto reply = reply_time_counters{};
    std::transform(std::begin(m_timecounters),
                   std::end(m_timecounters),
                   std::back_inserter(reply.counters),
                   [](const auto& tc) { return (to_timecounter(*tc)); });
    return (reply);
}

static std::chrono::nanoseconds get_clock_error(const clock& c)
{
    auto error =
        c.local_frequency_error().value_or(c.frequency_error().value_or(0));

    return (chrono::maximum_clock_error(error));
}

reply_msg server::handle_request(const request_time_keeper&)
{
    auto keeper = time_keeper{
        .ts = chrono::realtime::now(),
        .ts_error = get_clock_error(*m_clock),
        .info = {.freq = m_clock->frequency(),
                 .freq_error = m_clock->frequency_error(),
                 .local_freq = m_clock->local_frequency(),
                 .local_freq_error = m_clock->local_frequency_error(),
                 .offset = m_clock->offset(),
                 .theta = m_clock->theta(),
                 .synced = m_clock->synced()},
        .stats =
            {.freq_accept =
                 static_cast<int64_t>(m_clock->nb_frequency_accept()),
             .freq_reject =
                 static_cast<int64_t>(m_clock->nb_frequency_reject()),
             .local_freq_accept =
                 static_cast<int64_t>(m_clock->nb_local_frequency_accept()),
             .local_freq_reject =
                 static_cast<int64_t>(m_clock->nb_local_frequency_reject()),
             .theta_accept = static_cast<int64_t>(m_clock->nb_theta_accept()),
             .theta_reject = static_cast<int64_t>(m_clock->nb_theta_reject()),
             .timestamps = static_cast<int64_t>(m_clock->nb_timestamps()),
             .rtts = {.maximum = m_clock->rtt_maximum(),
                      .median = m_clock->rtt_median(),
                      .minimum = m_clock->rtt_minimum(),
                      .size = static_cast<int64_t>(m_clock->rtt_size())}},
    };

    core::to_string(m_timecounters.front()->id)
        .copy(keeper.counter_id, name_max_length);

    if (!m_sources.empty()) {
        m_sources.begin()->first.copy(keeper.source_id, name_max_length);
    }

    return (reply_time_keeper{keeper});
}

static time_source to_time_source(std::string_view id,
                                  const server::time_source& source)
{
    auto ts = time_source{};
    id.copy(ts.id, id_max_length);
    std::visit(
        [&](const auto& source) { ts.info = source::to_time_source(*source); },
        source);

    return (ts);
}

reply_msg server::handle_request(const request_time_sources& request)
{
    if (request.id) {
        auto item = m_sources.find(*request.id);
        if (item == std::end(m_sources)) {
            return (to_error(error_type::NOT_FOUND));
        }

        auto reply = reply_time_sources{};
        reply.sources.emplace_back(to_time_source(item->first, item->second));
        return (reply);
    }

    auto reply = reply_time_sources{};
    std::transform(std::begin(m_sources),
                   std::end(m_sources),
                   std::back_inserter(reply.sources),
                   [](const auto& item) {
                       return (to_time_source(item.first, item.second));
                   });

    return (reply);
}

static tl::expected<server::time_source, reply_error> make_time_source(
    const request_time_source_add& add, core::event_loop& loop, clock* clock)
{
    auto result = std::visit(
        utils::overloaded_visitor(
            [&](const time_source_ntp& ntp)
                -> tl::expected<server::time_source, reply_error> {
                addrinfo* ai = nullptr;
                if (auto error = getaddrinfo(
                        ntp.config.node, ntp.config.port, nullptr, &ai);
                    error != 0) {
                    return (tl::make_unexpected(
                        to_error(error_type::EAI_ERROR, error)));
                }

                return (std::make_unique<source::ntp>(loop, clock, ai));
            },
            [&](const time_source_system& sys)
                -> tl::expected<server::time_source, reply_error> {
                return (std::make_unique<source::system>(loop, clock));
            }),
        add.source.info);

    return (result);
}

reply_msg server::handle_request(const request_time_source_add& add)
{
    /* Since there "can be only one", delete all pre-existing sources */
    m_sources.clear();

    /* Clear any pre-existing clock data */
    m_clock->reset();

    auto source = make_time_source(add, m_loop, m_clock.get());
    if (!source) { return (source.error()); }

    auto [item, success] =
        m_sources.emplace(std::string{add.source.id}, std::move(*source));
    if (!success) { throw std::runtime_error("Could not create time source!"); }

    return (handle_request(request_time_sources{.id = add.source.id}));
}

reply_msg server::handle_request(const request_time_source_del& del)
{
    m_sources.erase(del.id);
    return (reply_ok{});
}

static int handle_rpc_request(const op_event_data* data, void* arg)
{
    auto s = reinterpret_cast<server*>(arg);

    auto reply_errors = 0;
    while (auto request = message::recv(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        auto request_visitor = [&](auto& request) -> reply_msg {
            return (s->handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);

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

server::server(void* context, openperf::core::event_loop& loop)
    : m_loop(loop)
    , m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.data()))
{
    /* Initialize and setup timecounter */
    counter::timecounter::make_all(m_timecounters);
    assert(!m_timecounters.empty());
    counter::timecounter_now.store(m_timecounters.front().get(),
                                   std::memory_order_release);

    /* Initialize and setup timehands */
    OP_LOG(OP_LOG_DEBUG,
           "Timesync service is using the %s timecounter\n",
           m_timecounters.front()->name().data());
    chrono::keeper::instance().setup(m_timecounters.front().get());

    /* Instantiate clock */
    m_clock = std::make_unique<clock>(
        [](const bintime& timestamp, counter::ticks ticks, counter::hz freq) {
            return (chrono::keeper::instance().sync(timestamp, ticks, freq));
        });

    /* Setup event loop */
    auto callbacks = op_event_callbacks{.on_read = handle_rpc_request};
    loop.add(m_socket.get(), &callbacks, this);
}

} // namespace openperf::timesync::api
