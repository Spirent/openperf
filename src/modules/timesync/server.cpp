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

static constexpr auto poll_period_adjust = 4s;

static time_counter to_timecounter(const counter::timecounter& tc)
{
    auto counter = time_counter{.frequency = tc.frequency().count(),
                                .priority = tc.static_priority()};

    core::to_string(tc.id).copy(counter.id, id_max_length);
    tc.name().copy(counter.name, name_max_length);
    return (counter);
}

static void* get_address(const struct addrinfo* ai)
{
    return (ai->ai_family == AF_INET
                ? static_cast<void*>(std::addressof(
                    reinterpret_cast<sockaddr_in*>(ai->ai_addr)->sin_addr))
                : static_cast<void*>(std::addressof(
                    reinterpret_cast<sockaddr_in6*>(ai->ai_addr)->sin6_addr)));
}

static in_port_t get_port(const struct addrinfo* ai)
{
    return (
        ntohs(ai->ai_family == AF_INET
                  ? reinterpret_cast<sockaddr_in*>(ai->ai_addr)->sin_port
                  : reinterpret_cast<sockaddr_in6*>(ai->ai_addr)->sin6_port));
}

std::pair<std::string, std::string> get_name_and_port(const struct addrinfo* ai)
{
    auto name = std::string{};
    auto port = std::string{};

    name.resize(name_max_length);
    port.resize(port_max_length);

    if (getnameinfo(ai->ai_addr,
                    ai->ai_addrlen,
                    name.data(),
                    name.size(),
                    port.data(),
                    port.size(),
                    NI_NAMEREQD)
        == 0) {
        name.resize(strnlen(name.data(), name.capacity()));
        port.resize(strnlen(port.data(), port.capacity()));

        return (std::make_pair(name, port));
    }

    /* Resolution failed for some reason; return the raw socket data */
    static_assert(name_max_length > INET6_ADDRSTRLEN);

    if (inet_ntop(
            ai->ai_family, get_address(ai), name.data(), ai->ai_addrlen)) {
        name.resize(strnlen(name.data(), name.capacity()));
        return (std::make_pair(name, std::to_string(get_port(ai))));
    }

    /* All forms of resolution failed */
    return (std::make_pair("unknown", "unknown"));
}

/**
 * Get the poll delay for the i'th packet.
 * This formula gives us a nice exponential back-off for
 * i = [0, ntp_startup_packets] which allows us to quickly get
 * lots of NTP timestamps when we start while slowing our polling
 * rate down afterwards.
 */
std::chrono::nanoseconds ntp_poll_delay(unsigned i,
                                        std::chrono::seconds max_period)
{
    using seconds = std::chrono::duration<double>;
    static constexpr int ntp_startup_packets = 8;

    auto period = seconds(
        i < ntp_startup_packets ? std::pow(
            std::exp(std::log(max_period.count()) / ntp_startup_packets), i)
                                : max_period.count());

    return (std::chrono::duration_cast<std::chrono::nanoseconds>(period));
}

static void handle_invalid_ntp_reply(ntp_server_state* state,
                                     ntp::packet& reply)
{
    auto np = get_name_and_port(state->addrinfo.get());
    switch (static_cast<ntp::kiss_code>(reply.refid)) {
    case ntp::kiss_code::KISS_DENY:
    case ntp::kiss_code::KISS_RSTR:
        OP_LOG(OP_LOG_WARNING,
               "NTP server %s has denied access; ceasing to poll\n",
               np.first.c_str());
        state->stats.poll_period =
            std::numeric_limits<std::chrono::seconds>::max();
        break;
    case ntp::kiss_code::KISS_RATE:
        OP_LOG(OP_LOG_WARNING,
               "Received RATE notification from NTP server %s; "
               "increasing poll period %ld -> %ld\n",
               np.first.c_str(),
               state->stats.poll_period.count(),
               (state->stats.poll_period + poll_period_adjust).count());
        state->stats.poll_period += poll_period_adjust;
        break;
    default: {
        /* refid contains a 4 octet ASCII string in network order; ugh */
        auto refid = ntohl(reply.refid);
        OP_LOG(OP_LOG_WARNING,
               "Received invalid reply from NTP server %s; kiss code = %.4s\n",
               np.first.c_str(),
               reinterpret_cast<const char*>(&refid));
    }
    }
}

static int handle_ntp_reply(const struct op_event_data*, void* arg)
{
    auto* state = reinterpret_cast<ntp_server_state*>(arg);

    auto buffer = std::array<std::byte, ntp::packet_size>();
    auto length = buffer.size();
    while (auto rx_time = state->socket.recv(buffer.data(), length)) {
        assert(length <= buffer.size());
        auto reply = ntp::deserialize(buffer.data(), length);

        /* Skip replies we can't decode. */
        if (!reply) { continue; }

        /*
         * According to RFC 5905, if the stratum is 0, then the
         * reply is invalid and the server might be trying to tell
         * us something.
         */
        if (!reply->stratum) {
            handle_invalid_ntp_reply(state, *reply);
            continue;
        }

        /*
         * Finally, verify that this is a reply to a request we sent
         * by confirming that it contains an origin that matches what
         * we expect.
         */
        if (reply->origin != state->expected_origin) {
            OP_LOG(OP_LOG_WARNING,
                   "Ignoring NTP reply with unrecognized origin timestamp, "
                   "%016" PRIx64 ".%016" PRIx64 "\n",
                   reply->origin.bt_sec,
                   reply->origin.bt_frac);
            continue;
        }

        // ntp::dump(stderr, *reply);

        state->stats.rx++;
        state->stats.stratum = reply->stratum;
        state->clock->update(
            state->last_tx, reply->receive, reply->transmit, *rx_time);
    }

    return (-1);
}

/*
 * Since NTP only uses 64 bits for the timestamp, we need to
 * trim our bintime values accordingly.
 */
static bintime to_ntp_bintime(const bintime& from)
{
    static constexpr int64_t lo_mask = 0xffffffff;
    static constexpr int64_t hi_mask = lo_mask << 32;
    return (bintime{.bt_sec = from.bt_sec & lo_mask,
                    .bt_frac = from.bt_frac & hi_mask});
}

static int handle_ntp_poll(const struct op_event_data* data, void* arg)
{
    auto* state = reinterpret_cast<ntp_server_state*>(arg);

    auto now = to_bintime(chrono::realtime::now().time_since_epoch());

    /*
     * Record this timestamp so we can verify replies. The server
     * should use our transmit timestamp for the origin timestamp.
     */
    state->expected_origin = to_ntp_bintime(now);

    /* Only need the mode and transmit timestamps for a client request */
    auto request = ntp::packet{.mode = ntp::mode::MODE_CLIENT, .transmit = now};

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
        static_cast<uint64_t>(
            ntp_poll_delay(state->stats.tx, state->stats.poll_period).count()));

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

tl::expected<time_source, int> to_time_source(std::string_view id,
                                              const ntp_server_state* state)
{
    auto config = time_source_config_ntp{};
    auto np = get_name_and_port(state->addrinfo.get());
    std::copy_n(np.first.data(), np.first.size(), config.node);
    std::copy_n(np.second.data(), np.second.size(), config.port);

    auto ts = time_source{
        .config = config,
        .stats = {
            .poll_period = std::chrono::duration_cast<std::chrono::seconds>(
                ntp_poll_delay(state->stats.tx, state->stats.poll_period)),
            .rx_packets = state->stats.rx,
            .tx_packets = state->stats.tx,
            .stratum = state->stats.stratum}};
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
            add.source.config.node, add.source.config.port, nullptr, &ai);
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
