#include <algorithm>
#include <iomanip>
#include <sstream>

#include <zmq.h>

#include "timesync/api.hpp"
#include "timesync/bintime.hpp"
#include "timesync/chrono.hpp"
#include "timesync/counter.hpp"
#include "message/serialized_message.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

namespace openperf::timesync::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request_time_counters& counters) {
                     return (
                         counters.id.has_value()
                             ? message::push(serialized, counters.id.value())
                             : message::push(serialized,
                                             std::string())); // empty string
                 },
                 [&](const request_time_keeper&) { return (0); },
                 [&](const request_time_sources& sources) {
                     return (
                         sources.id.has_value()
                             ? message::push(serialized, sources.id.value())
                             : message::push(serialized,
                                             std::string())); // empty string
                 },
                 [&](const request_time_source_add& add) {
                     return (message::push(serialized, add.source));
                 },
                 [&](const request_time_source_del& del) {
                     return (message::push(
                         serialized, del.id.data(), del.id.length()));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const reply_time_counters& counters) {
                     return (message::push(serialized, counters.counters));
                 },
                 [&](const reply_time_keeper& keeper) {
                     return (message::push(serialized, keeper.keeper));
                 },
                 [&](const reply_time_sources& sources) {
                     return (message::push(serialized, sources.sources));
                 },
                 [&](const reply_ok&) { return (0); },
                 [&](const reply_error& error) {
                     return (message::push(serialized, error.info));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<request_msg, request_time_counters>(): {
        auto id = message::pop_string(msg);
        if (!id.empty()) { return (request_time_counters{std::move(id)}); }
        return (request_time_counters{});
    }
    case utils::variant_index<request_msg, request_time_keeper>():
        return (request_time_keeper{});
    case utils::variant_index<request_msg, request_time_sources>(): {
        auto id = message::pop_string(msg);
        if (!id.empty()) { return (request_time_sources{std::move(id)}); }
        return (request_time_sources{});
    }
    case utils::variant_index<request_msg, request_time_source_add>():
        return (request_time_source_add{message::pop<time_source>(msg)});
    case utils::variant_index<request_msg, request_time_source_del>(): {
        return (request_time_source_del{message::pop_string(msg)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_time_counters>(): {
        return (reply_time_counters{message::pop_vector<time_counter>(msg)});
    }
    case utils::variant_index<reply_msg, reply_time_keeper>():
        return (reply_time_keeper{message::pop<time_keeper>(msg)});
    case utils::variant_index<reply_msg, reply_time_sources>(): {
        return (reply_time_sources{message::pop_vector<time_source>(msg)});
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{message::pop<typed_error>(msg)});
    }

    return (tl::make_unexpected(EINVAL));
}

std::shared_ptr<swagger::v1::model::TimeCounter>
to_swagger(const time_counter& src)
{
    using TimeCounter = swagger::v1::model::TimeCounter;
    auto dst = std::make_shared<TimeCounter>();

    dst->setId(std::string(src.id));
    dst->setName(std::string(src.name));
    dst->setFrequency(src.frequency);
    dst->setPriority(src.priority);

    return (dst);
}

template <typename Rep, typename Period>
std::string to_rfc3339(std::chrono::duration<Rep, Period> from)
{
    auto tv = to_timeval(from);
    std::stringstream os;
    os << std::put_time(gmtime(&tv.tv_sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << "Z";
    return (os.str());
}

std::shared_ptr<swagger::v1::model::TimeKeeperState>
to_swagger(const time_keeper_info& src)
{
    using TimeKeeperState = swagger::v1::model::TimeKeeperState;
    auto dst = std::make_shared<TimeKeeperState>();

    if (src.freq) { dst->setFrequency(src.freq->count()); }
    if (src.freq_error) { dst->setFrequencyError(src.freq_error->count()); }
    if (src.local_freq) { dst->setLocalFrequency(src.local_freq->count()); }
    if (src.local_freq) {
        dst->setLocalFrequencyError(src.local_freq_error->count());
    }
    dst->setOffset(to_double(src.offset));
    dst->setSynced(src.synced);
    if (src.theta) { dst->setTheta(to_double(*src.theta)); }

    return (dst);
}

std::shared_ptr<swagger::v1::model::TimeKeeperStats>
to_swagger(const time_keeper_stats& src)
{
    using RttStats = swagger::v1::model::TimeKeeperStats_round_trip_times;
    auto rtt_stats = std::make_shared<RttStats>();

    if (src.rtts.median) { rtt_stats->setAvg(*src.rtts.median); }
    if (src.rtts.maximum) { rtt_stats->setMax(*src.rtts.maximum); }
    if (src.rtts.minimum) { rtt_stats->setMin(*src.rtts.minimum); }
    rtt_stats->setSize(src.rtts.size);

    using TimeKeeperStats = swagger::v1::model::TimeKeeperStats;
    auto dst = std::make_shared<TimeKeeperStats>();

    dst->setFrequencyAccept(src.freq_accept);
    dst->setFrequencyReject(src.freq_reject);
    dst->setLocalFrequencyAccept(src.local_freq_accept);
    dst->setLocalFrequencyReject(src.local_freq_reject);
    dst->setRoundTripTimes(rtt_stats);
    dst->setThetaAccept(src.theta_accept);
    dst->setThetaReject(src.theta_reject);
    dst->setTimestamps(src.timestamps);

    return (dst);
}

std::shared_ptr<swagger::v1::model::TimeKeeper>
to_swagger(const time_keeper& src)
{
    using TimeKeeper = swagger::v1::model::TimeKeeper;
    auto dst = std::make_shared<TimeKeeper>();

    dst->setState(to_swagger(src.info));
    dst->setStats(to_swagger(src.stats));
    dst->setTime(to_rfc3339(src.ts.time_since_epoch()));
    dst->setTimeCounterId(std::string(src.counter_id));
    dst->setTimeSourceId(std::string(src.source_id));

    return (dst);
}

std::shared_ptr<swagger::v1::model::TimeSourceConfig>
to_swagger(const time_source_config_ntp& src)
{
    using TimeSourceConfig_ntp = swagger::v1::model::TimeSourceConfig_ntp;
    auto ntp_conf = std::make_shared<TimeSourceConfig_ntp>();

    ntp_conf->setHostname(src.node);
    ntp_conf->setPort(src.port);

    using TimeSourceConfig = swagger::v1::model::TimeSourceConfig;
    auto dst = std::make_shared<TimeSourceConfig>();
    dst->setNtp(ntp_conf);
    return (dst);
}

std::shared_ptr<swagger::v1::model::TimeSourceStats>
to_swagger(const time_source_stats_ntp& src)
{
    using TimeSourceStats_ntp = swagger::v1::model::TimeSourceStats_ntp;
    auto ntp_stats = std::make_shared<TimeSourceStats_ntp>();

    ntp_stats->setRxPackets(src.rx_packets);
    ntp_stats->setTxPackets(src.tx_packets);
    if (src.stratum) { ntp_stats->setStratum(*src.stratum); }

    using TimeSourceStats = swagger::v1::model::TimeSourceStats;
    auto dst = std::make_shared<TimeSourceStats>();
    dst->setNtp(ntp_stats);
    return (dst);
}

std::shared_ptr<swagger::v1::model::TimeSource>
to_swagger(const time_source& src)
{
    using TimeSource = swagger::v1::model::TimeSource;
    auto dst = std::make_shared<TimeSource>();

    dst->setId(std::string(src.id));
    dst->setKind("ntp");
    dst->setConfig(to_swagger(src.config));
    dst->setStats(to_swagger(src.stats));

    return (dst);
}

time_source_config_ntp
from_swagger(const swagger::v1::model::TimeSourceConfig_ntp& src)
{
    auto to_return = time_source_config_ntp{};
    src.getHostname().copy(to_return.node, name_max_length);

    if (src.portIsSet()) {
        src.getPort().copy(to_return.port, port_max_length);
    } else {
        static constexpr std::string_view port = "ntp";
        port.copy(to_return.port, port_max_length);
    }

    return (to_return);
}

time_source from_swagger(const swagger::v1::model::TimeSource& src)
{
    auto to_return = time_source{};
    if (src.getKind() != "ntp") { return (to_return); }

    src.getId().copy(to_return.id, id_max_length);

    auto config = src.getConfig();
    if (config) { to_return.config = from_swagger(*config->getNtp()); }

    return (to_return);
}

reply_error to_error(error_type type, int value)
{
    return (reply_error{.info = typed_error{.type = type, .value = value}});
}

} // namespace openperf::timesync::api

namespace swagger::v1::model {

void from_json(const nlohmann::json& j, TimeSource& ts)
{
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        ts.setId(j["id"]);
    } else {
        ts.setId(openperf::core::to_string(openperf::core::uuid::random()));
    }

    if (j.find("kind") != j.end() && !j["kind"].is_null()) {
        ts.setKind(j["kind"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<TimeSourceConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        ts.setConfig(config);
    }

    if (j.find("stats") != j.end() && !j["stats"].is_null()) {
        auto stats = std::make_shared<TimeSourceStats>();
        stats->fromJson(const_cast<nlohmann::json&>(j["stats"]));
        ts.setStats(stats);
    }
}

} // namespace swagger::v1::model
