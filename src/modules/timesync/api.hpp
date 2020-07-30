#ifndef _OP_TIMESYNC_API_HPP_
#define _OP_TIMESYNC_API_HPP_

#include <chrono>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <netdb.h>

#include <zmq.h>
#include <tl/expected.hpp>

#include "timesync/chrono.hpp"
#include "swagger/v1/model/TimeCounter.h"
#include "swagger/v1/model/TimeKeeper.h"
#include "swagger/v1/model/TimeSource.h"

namespace openperf::message {
struct serialized_message;
}

namespace openperf::timesync::api {

inline constexpr std::string_view endpoint = "inproc://openperf_timesync";

static constexpr size_t id_max_length = 64;
static constexpr size_t name_max_length = NI_MAXHOST;
static constexpr size_t port_max_length = NI_MAXSERV;

using serialized_msg = openperf::message::serialized_message;

struct time_counter
{
    char id[id_max_length];
    char name[name_max_length];
    uint64_t frequency;
    int priority;
};

struct request_time_counters
{
    std::optional<std::string> id = std::nullopt;
};

struct reply_time_counters
{
    std::vector<time_counter> counters;
};

struct request_time_keeper
{};

struct time_keeper_info
{
    std::optional<counter::hz> freq;
    std::optional<counter::hz> freq_error;
    std::optional<counter::hz> local_freq;
    std::optional<counter::hz> local_freq_error;
    bintime offset;
    std::optional<bintime> theta;
    bool synced;
};

struct time_keeper_rtt_stats
{
    std::optional<double> maximum;
    std::optional<double> median;
    std::optional<double> minimum;
    int64_t size;
};

struct time_keeper_stats
{
    int64_t freq_accept;
    int64_t freq_reject;
    int64_t local_freq_accept;
    int64_t local_freq_reject;
    int64_t theta_accept;
    int64_t theta_reject;
    int64_t timestamps;
    time_keeper_rtt_stats rtts;
};

struct time_keeper
{
    using time_point = std::chrono::time_point<chrono::realtime>;
    char counter_id[id_max_length];
    char source_id[id_max_length];
    time_point ts;
    time_keeper_info info;
    time_keeper_stats stats;
};

struct reply_time_keeper
{
    time_keeper keeper;
};

struct time_source_config_ntp
{
    char node[name_max_length];
    char port[port_max_length];
};

struct time_source_stats_ntp
{
    int64_t rx_packets;
    int64_t tx_packets;
    std::optional<uint8_t> stratum;
};

struct time_source
{
    char id[id_max_length];
    time_source_config_ntp config;
    time_source_stats_ntp stats;
};

struct request_time_sources
{
    std::optional<std::string> id = std::nullopt;
};

struct reply_time_sources
{
    std::vector<time_source> sources;
};

struct request_time_source_add
{
    time_source source;
};

struct request_time_source_del
{
    std::string id;
};

using request_msg = std::variant<request_time_counters,
                                 request_time_keeper,
                                 request_time_sources,
                                 request_time_source_add,
                                 request_time_source_del>;

struct reply_ok
{};

enum class error_type { NONE = 0, NOT_FOUND, EAI_ERROR, ZMQ_ERROR };

struct typed_error
{
    error_type type = error_type::NONE;
    int value = 0;
};

struct reply_error
{
    typed_error info;
};

using reply_msg = std::variant<reply_time_counters,
                               reply_time_keeper,
                               reply_time_sources,
                               reply_ok,
                               reply_error>;

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

std::shared_ptr<swagger::v1::model::TimeCounter>
to_swagger(const time_counter&);
std::shared_ptr<swagger::v1::model::TimeKeeper> to_swagger(const time_keeper&);
std::shared_ptr<swagger::v1::model::TimeSource> to_swagger(const time_source&);

time_source from_swagger(const swagger::v1::model::TimeSource&);

reply_error to_error(error_type type, int value = 0);

} // namespace openperf::timesync::api

namespace swagger::v1::model {
void from_json(const nlohmann::json&, swagger::v1::model::TimeSource&);
}

#endif /* _OP_TIMESYNC_API_HPP_ */
