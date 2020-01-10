#include <algorithm>
#include <cmath>

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

/**
 * Get the poll delay for the i'th packet.
 * This formula gives us a nice exponential back-off for
 * i = [0, ntp_startup_packets] which allows us to quickly get
 * lots of NTP timestamps when we start while slowing our polling
 * rate down afterwards.
 */
std::chrono::nanoseconds ntp_poll_delay(unsigned i)
{
    using seconds = std::chrono::duration<double>;
    static constexpr int ntp_startup_packets = 8;
    static constexpr auto ntp_poll_period = seconds(64);

    auto period = seconds(
        i < ntp_startup_packets ? std::pow(
            std::exp(std::log(ntp_poll_period.count()) / ntp_startup_packets),
            i)
                                : ntp_poll_period.count());

    return (std::chrono::duration_cast<std::chrono::nanoseconds>(period));
}

static int handle_ntp_reply(const struct op_event_data*, void* arg)
{
    auto state = reinterpret_cast<ntp_server_state*>(arg);

    auto buffer = std::array<std::byte, ntp::packet_size>();
    auto length = buffer.size();
    while (auto rx_time = state->socket.recv(buffer.data(), length)) {
        assert(length <= buffer.size());
        auto reply = ntp::deserialize(buffer.data(), length);
        if (!reply) continue;
        // ntp::dump(stderr, *reply);
        state->stats.rx++;
        state->clock->update(
            state->last_tx, reply->receive, reply->transmit, *rx_time);
    }

    return (-1);
}

static int handle_ntp_poll(const struct op_event_data* data, void* arg)
{
    auto state = reinterpret_cast<ntp_server_state*>(arg);

    auto request = ntp::packet{.leap = ntp::leap_status::LEAP_UNKNOWN,
                               .mode = ntp::mode::MODE_CLIENT,
                               .stratum = 0,
                               .poll = 4,
                               .precision = -6,
                               .root = {
                                   .delay = {.bt_sec = 1, .bt_frac = 0},
                                   .dispersion = {.bt_sec = 1, .bt_frac = 0},
                               }};

    auto buffer = ntp::serialize(request);

    auto reply_callbacks = op_event_callbacks{.on_read = handle_ntp_reply};
    op_event_loop_add(data->loop, state->socket.fd(), &reply_callbacks, arg);

    auto tx_time = state->socket.send(buffer.data(), buffer.size());
    if (!tx_time) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to send NTP request: %s\n",
               strerror(tx_time.error()));
        op_event_loop_del(data->loop, state->socket.fd());
        return (-1);
    }

    state->last_tx = *tx_time;
    state->stats.tx++;

    op_event_loop_update(
        data->loop,
        state->poll_loop_id,
        static_cast<uint64_t>(ntp_poll_delay(state->stats.tx).count()));

    return (0);
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

reply_msg server::handle_request(const request_time_keeper&)
{
    auto keeper = time_keeper{
        .ts = chrono::realtime::now(),
        .info = {.freq = m_clock->frequency(),
                 .freq_error = m_clock->frequency_error(),
                 .local_freq = m_clock->local_frequency(),
                 .local_freq_error = m_clock->local_frequency_error(),
                 .offset = m_clock->offset(),
                 .synced = m_clock->synced(),
                 .theta = m_clock->theta()},
        .stats = {.freq = static_cast<int64_t>(m_clock->nb_frequency_updates()),
                  .local_freq = static_cast<int64_t>(
                      m_clock->nb_local_frequency_updates()),
                  .rtts = {.maximum = m_clock->rtt_maximum(),
                           .median = m_clock->rtt_median(),
                           .minimum = m_clock->rtt_minimum()},
                  .theta = static_cast<int64_t>(m_clock->nb_theta_updates()),
                  .timestamps = static_cast<int64_t>(m_clock->nb_timestamps()),
                  .updates = static_cast<int64_t>(m_clock->nb_updates())},
    };

    core::to_string(m_timecounters.front()->id)
        .copy(keeper.counter_id, name_max_length);

    if (!m_sources.empty()) {
        m_sources.begin()->first.copy(keeper.source_id, name_max_length);
    }

    return (reply_time_keeper{keeper});
}

tl::expected<time_source, int> to_time_source(std::string_view id,
                                              const ntp_server_state* state)
{
    std::array<char, name_max_length> node;
    std::array<char, service_max_length> service;

    auto ai = state->addrinfo.get();
    auto error = getnameinfo(ai->ai_addr,
                             ai->ai_addrlen,
                             node.data(),
                             node.size(),
                             service.data(),
                             service.size(),
                             NI_NAMEREQD);
    if (error) { return (tl::make_unexpected(error)); }

    auto config = time_source_config_ntp{};
    std::copy_n(node.data(),
                std::min(std::strlen(node.data()), node.size()),
                config.node);
    std::copy_n(service.data(),
                std::min(std::strlen(service.data()), service.size()),
                config.service);

    auto ts = time_source{.config = config,
                          .stats = {state->stats.rx, state->stats.tx}};
    id.copy(ts.id, id_max_length);

    return (ts);
}

reply_msg server::handle_request(const request_time_sources& request)
{
    if (request.id) {
        auto item = m_sources.find(*request.id);
        if (item == std::end(m_sources)) {
            return (to_error(error_type::NOT_FOUND));
        }

        auto ts = to_time_source(item->first, item->second.get());
        if (!ts) { return (to_error(error_type::EAI_ERROR, ts.error())); }

        auto reply = reply_time_sources{};
        reply.sources.emplace_back(*ts);
        return (reply);
    }

    auto reply = reply_time_sources{};
    int error = 0;
    for (const auto& item : m_sources) {
        auto ts = to_time_source(item.first, item.second.get());
        if (ts) {
            reply.sources.emplace_back(*ts);
        } else {
            error = ts.error();
        }
    }

    if (error) { return (to_error(error_type::EAI_ERROR, error)); }

    return (reply);
}

reply_msg server::handle_request(const request_time_source_add& add)
{
    using namespace std::chrono_literals;

    addrinfo* ai = nullptr;
    if (auto error = getaddrinfo(
            add.source.config.node, add.source.config.service, nullptr, &ai);
        error != 0) {
        return (to_error(error_type::EAI_ERROR, error));
    }

    /* Since there "can be only one", delete all pre-existing sources */
    for (auto& item : m_sources) {
        auto state = item.second.get();
        m_loop.del(state->poll_loop_id);
        m_loop.del(state->socket.fd());
    }
    m_sources.clear();

    /* Clear any pre-existing clock data */
    m_clock->reset();

    auto item = m_sources.emplace(
        add.source.id, std::make_unique<ntp_server_state>(ai, m_clock.get()));
    auto state = item.first->second.get();

    auto callbacks = op_event_callbacks{.on_timeout = handle_ntp_poll};

    m_loop.add((100ns).count(), &callbacks, state, &(state->poll_loop_id));

    return (handle_request(request_time_sources{.id = add.source.id}));
}

reply_msg server::handle_request(const request_time_source_del& del)
{
    if (auto item = m_sources.find(del.id); item != m_sources.end()) {
        auto state = item->second.get();
        m_loop.del(state->poll_loop_id);
        m_loop.del(state->socket.fd());
        m_sources.erase(item);
    }

    return (reply_ok{});
}

static int handle_rpc_request(const op_event_data* data, void* arg)
{
    auto s = reinterpret_cast<server*>(arg);

    auto reply_errors = 0;
    while (auto request = recv_message(data->socket, ZMQ_DONTWAIT)
                              .and_then(deserialize_request)) {
        auto request_visitor = [&](auto& request) -> reply_msg {
            return (s->handle_request(request));
        };
        auto reply = std::visit(request_visitor, *request);

        if (send_message(data->socket, serialize_reply(reply)) == -1) {
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
