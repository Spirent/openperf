#include <algorithm>
#include <iomanip>
#include <sstream>

#include <zmq.h>

#include "timesync/api.hpp"
#include "timesync/bintime.hpp"
#include "timesync/chrono.hpp"
#include "timesync/counter.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

namespace openperf::timesync::api {

template <typename T> static auto zmq_msg_init(zmq_msg_t* msg, const T& value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(msg));
    *ptr = value;
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, const T* buffer, size_t length)
{
    if (auto error = zmq_msg_init_size(msg, length); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<char*>(zmq_msg_data(msg));
    auto src = reinterpret_cast<const char*>(buffer);
    std::copy(src, src + length, ptr);
    return (0);
}

template <typename T> static T zmq_msg_data(const zmq_msg_t* msg)
{
    return (reinterpret_cast<T>(zmq_msg_data(const_cast<zmq_msg_t*>(msg))));
}

template <typename T> static size_t zmq_msg_size(const zmq_msg_t* msg)
{
    return (zmq_msg_size(const_cast<zmq_msg_t*>(msg)) / sizeof(T));
}

static void close(serialized_msg& msg)
{
    zmq_close(&msg.type);
    zmq_close(&msg.data);
}

serialized_msg serialize_request(const request_msg& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const request_time_counters& counters) {
                     return (counters.id ? zmq_msg_init(&serialized.data,
                                                        counters.id->data(),
                                                        counters.id->length())
                                         : zmq_msg_init(&serialized.data));
                 },
                 [&](const request_time_keeper&) {
                     return (zmq_msg_init(&serialized.data));
                 },
                 [&](const request_time_sources& sources) {
                     return (sources.id ? zmq_msg_init(&serialized.data,
                                                       sources.id->data(),
                                                       sources.id->length())
                                        : zmq_msg_init(&serialized.data));
                 },
                 [&](const request_time_source_add& add) {
                     return (zmq_msg_init(&serialized.data,
                                          std::addressof(add.source),
                                          sizeof(add.source)));
                 },
                 [&](const request_time_source_del& del) {
                     return (zmq_msg_init(
                         &serialized.data, del.id.data(), del.id.length()));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(const reply_msg& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](const reply_time_counters& counters) {
                     /*
                      * ZMQ wants the length in bytes, so we have to scale the
                      * length of the vector up to match.
                      */
                     auto scalar =
                         sizeof(decltype(counters.counters)::value_type);
                     return (zmq_msg_init(&serialized.data,
                                          counters.counters.data(),
                                          scalar * counters.counters.size()));
                 },
                 [&](const reply_time_keeper& keeper) {
                     return (zmq_msg_init(&serialized.data, keeper.keeper));
                 },
                 [&](const reply_time_sources& sources) {
                     auto scalar =
                         sizeof(decltype(sources.sources)::value_type);
                     return (zmq_msg_init(&serialized.data,
                                          sources.sources.data(),
                                          scalar * sources.sources.size()));
                 },
                 [&](const reply_ok&) {
                     return (zmq_msg_init(&serialized.data, 0));
                 },
                 [&](const reply_error& error) {
                     return (zmq_msg_init(&serialized.data, error.info));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<request_msg, request_time_counters>(): {
        if (zmq_msg_size(&msg.data)) {
            std::string id(zmq_msg_data<char*>(&msg.data),
                           zmq_msg_size(&msg.data));
            return (request_time_counters{std::move(id)});
        }
        return (request_time_counters{});
    }
    case utils::variant_index<request_msg, request_time_keeper>():
        return (request_time_keeper{});
    case utils::variant_index<request_msg, request_time_sources>(): {
        if (zmq_msg_size(&msg.data)) {
            std::string id(zmq_msg_data<char*>(&msg.data),
                           zmq_msg_size(&msg.data));
            return (request_time_sources{std::move(id)});
        }
        return (request_time_sources{});
    }
    case utils::variant_index<request_msg, request_time_source_add>():
        return (
            request_time_source_add{*(zmq_msg_data<time_source*>(&msg.data))});
    case utils::variant_index<request_msg, request_time_source_del>(): {
        std::string id(zmq_msg_data<char*>(&msg.data), zmq_msg_size(&msg.data));
        return (request_time_source_del{std::move(id)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = *(zmq_msg_data<index_type*>(&msg.type));
    switch (idx) {
    case utils::variant_index<reply_msg, reply_time_counters>(): {
        auto data = zmq_msg_data<time_counter*>(&msg.data);
        std::vector<time_counter> counters(
            data, data + zmq_msg_size<time_counter>(&msg.data));
        return (reply_time_counters{std::move(counters)});
    }
    case utils::variant_index<reply_msg, reply_time_keeper>():
        return (reply_time_keeper{*(zmq_msg_data<time_keeper*>(&msg.data))});
    case utils::variant_index<reply_msg, reply_time_sources>(): {
        auto data = zmq_msg_data<time_source*>(&msg.data);
        std::vector<time_source> sources(
            data, data + zmq_msg_size<time_source>(&msg.data));
        return (reply_time_sources{std::move(sources)});
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{*(zmq_msg_data<typed_error*>(&msg.data))});
    }

    return (tl::make_unexpected(EINVAL));
}

int send_message(void* socket, serialized_msg&& msg)
{
    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.data, socket, 0) == -1) {
        close(msg);
        return (errno);
    }

    return (0);
}

tl::expected<serialized_msg, int> recv_message(void* socket, int flags)
{
    serialized_msg msg;
    if (zmq_msg_init(&msg.type) == -1 || zmq_msg_init(&msg.data) == -1) {
        close(msg);
        return (tl::make_unexpected(ENOMEM));
    }

    if (zmq_msg_recv(&msg.type, socket, flags) == -1
        || zmq_msg_recv(&msg.data, socket, flags) == -1) {
        close(msg);
        return (tl::make_unexpected(errno));
    }

    return (msg);
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

std::shared_ptr<swagger::v1::model::TimeKeeperInfo>
to_swagger(const time_keeper_info& src)
{
    using TimeKeeperInfo = swagger::v1::model::TimeKeeperInfo;
    auto dst = std::make_shared<TimeKeeperInfo>();

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

    dst->setTime(to_rfc3339(src.ts.time_since_epoch()));
    dst->setTimeCounterId(std::string(src.counter_id));
    dst->setTimeSourceId(std::string(src.source_id));
    dst->setInfo(to_swagger(src.info));
    dst->setStats(to_swagger(src.stats));

    return (dst);
}

std::shared_ptr<swagger::v1::model::TimeSourceConfig>
to_swagger(const time_source_config_ntp& src)
{
    using TimeSourceConfig_ntp = swagger::v1::model::TimeSourceConfig_ntp;
    auto ntp_conf = std::make_shared<TimeSourceConfig_ntp>();

    ntp_conf->setHostname(src.node);
    ntp_conf->setService(src.service);

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

    if (src.serviceIsSet()) {
        src.getService().copy(to_return.service, service_max_length);
    } else {
        static constexpr std::string_view service = "ntp";
        service.copy(to_return.service, service_max_length);
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
