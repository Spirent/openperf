#include <iomanip>
#include <sstream>

#include "basen.hpp"

#include "packet/analyzer/api.hpp"
#include "packet/analyzer/sink.hpp"
#include "packet/analyzer/statistics/flow/header_view.hpp"
#include "packet/protocol/transmogrify/protocols.hpp"
#include "packet/statistics/api_transmogrify.hpp"

#include "swagger/v1/model/PacketAnalyzer.h"
#include "swagger/v1/model/PacketAnalyzerResult.h"
#include "swagger/v1/model/RxFlow.h"

#include "utils/overloaded_visitor.hpp"

namespace openperf::packet::analyzer::api {

static void to_swagger(api::protocol_counters_config src,
                       std::vector<std::string>& dst)
{
    auto value = 1;
    while (value <= packet::statistics::all_protocol_counters.value) {
        auto flag = packet::statistics::protocol_flags{value};
        if (src & flag) { dst.emplace_back(packet::statistics::to_name(flag)); }
        value <<= 1;
    }
}

static void to_swagger(api::flow_counters_config src,
                       std::vector<std::string>& dst)
{
    auto value = 1;
    while (value <= statistics::all_flow_counters.value) {
        auto flag = statistics::flow_flags{value};
        if (src & flag) { dst.emplace_back(statistics::to_name(flag)); }
        value <<= 1;
    }
}

analyzer_ptr to_swagger(const sink& src)
{
    auto& src_config = src.get_config();
    auto dst = std::make_unique<swagger::v1::model::PacketAnalyzer>();

    dst->setId(src.id());
    dst->setSourceId(src.source());
    dst->setActive(src.active());

    auto dst_config =
        std::make_shared<swagger::v1::model::PacketAnalyzerConfig>();
    to_swagger(src_config.protocol_counters, dst_config->getProtocolCounters());
    to_swagger(src_config.flow_counters, dst_config->getFlowCounters());
    if (!src_config.filter.empty()) {
        dst_config->setFilter(src_config.filter);
    }

    dst->setConfig(dst_config);

    return (dst);
}

template <typename StatTuple, typename GenericCounter>
inline void maybe_add_counter_tuple(const GenericCounter& x,
                                    GenericCounter& sum)
{
    if (x.template holds<StatTuple>()) {
        sum.template get<StatTuple>() += x.template get<StatTuple>();
    }
}

template <typename StatTuple, typename GenericCounter>
inline void maybe_add_summary_tuple(const GenericCounter& x,
                                    GenericCounter& sum)
{
    using frame_counter = statistics::flow::frame_counter;

    if (x.template holds<StatTuple>()) {
        auto& sum_tuple = sum.template get<StatTuple>();
        auto sum_count = sum.template get<frame_counter>().count;
        auto& x_tuple = x.template get<StatTuple>();
        auto x_count = x.template get<frame_counter>().count;

        /*
         * XXX: We need the totals of each tuple to calculate the variance,
         * hence we update the variance sum before adding anything else.
         */
        sum_tuple.m2 = statistics::flow::add_variance(sum_count,
                                                      sum_tuple.total,
                                                      sum_tuple.m2,
                                                      x_count,
                                                      x_tuple.total,
                                                      x_tuple.m2);

        sum_tuple.max = std::max(sum_tuple.max, x_tuple.max);
        sum_tuple.min = std::min(sum_tuple.min, x_tuple.min);
        sum_tuple.total += x_tuple.total;
    }
}

template <typename StatTuple, typename GenericCounter>
inline void maybe_copy_counter_tuple(const GenericCounter& src,
                                     GenericCounter& dst)
{
    if (src.template holds<StatTuple>()) {
        dst.template get<StatTuple>() = src.template get<StatTuple>();
    }
}

inline packet::statistics::generic_protocol_counters
sum_counters(protocol_counters_config config,
             const std::vector<sink_result::protocol_shard>& src)
{
    using namespace openperf::packet::statistics::protocol;

    auto sum = make_counters(config);

    std::for_each(std::begin(src), std::end(src), [&](const auto& shard) {
        maybe_add_counter_tuple<ethernet>(shard, sum);
        maybe_add_counter_tuple<ip>(shard, sum);
        maybe_add_counter_tuple<transport>(shard, sum);
        maybe_add_counter_tuple<tunnel>(shard, sum);
        maybe_add_counter_tuple<inner_ethernet>(shard, sum);
        maybe_add_counter_tuple<inner_ip>(shard, sum);
        maybe_add_counter_tuple<inner_transport>(shard, sum);
    });

    return (sum);
}

inline void copy_flow_counters(const statistics::generic_flow_counters& src,
                               statistics::generic_flow_counters& dst)
{
    using namespace openperf::packet::analyzer::statistics::flow;

    /* basic counters are always present */
    const auto& src_frames = src.get<frame_counter>();

    do {
        dst.get<frame_counter>() = src_frames;
        maybe_copy_counter_tuple<errors>(src, dst);
        maybe_copy_counter_tuple<sequencing>(src, dst);
        maybe_copy_counter_tuple<frame_length>(src, dst);
        maybe_copy_counter_tuple<interarrival>(src, dst);
        maybe_copy_counter_tuple<jitter_ipdv>(src, dst);
        maybe_copy_counter_tuple<jitter_rfc>(src, dst);
        maybe_copy_counter_tuple<latency>(src, dst);
    } while (dst.get<frame_counter>().count != src_frames.count);

    maybe_copy_counter_tuple<header>(src, dst);
}

inline void add_flow_counters(const statistics::generic_flow_counters& x,
                              statistics::generic_flow_counters& sum)
{
    using namespace openperf::packet::analyzer::statistics;

    if (x.holds<flow::errors>()) {
        auto& sum_errors = sum.get<flow::errors>();
        const auto& x_errors = x.get<flow::errors>();

        sum_errors.fcs += x_errors.fcs;
        sum_errors.ipv4_checksum += x_errors.ipv4_checksum;
        sum_errors.tcp_checksum += x_errors.tcp_checksum;
        sum_errors.udp_checksum += x_errors.udp_checksum;
    }

    if (x.holds<flow::sequencing>()) {
        auto& sum_seq = sum.get<flow::sequencing>();
        const auto& x_seq = x.get<flow::sequencing>();

        sum_seq.dropped += x_seq.dropped;
        sum_seq.duplicate += x_seq.duplicate;
        sum_seq.late += x_seq.late;
        sum_seq.reordered += x_seq.reordered;
        sum_seq.in_order += x_seq.in_order;
    }

    maybe_add_summary_tuple<flow::frame_length>(x, sum);
    maybe_add_summary_tuple<flow::interarrival>(x, sum);
    maybe_add_summary_tuple<flow::jitter_ipdv>(x, sum);
    maybe_add_summary_tuple<flow::jitter_rfc>(x, sum);
    maybe_add_summary_tuple<flow::latency>(x, sum);

    /*
     * XXX: Sum the counter structure last.  "Adding" the summary tuples
     * together requires the frame count to be correct for each tuple.
     */
    sum.get<flow::frame_counter>() += x.get<flow::frame_counter>();
}

inline statistics::generic_flow_counters
sum_counters(flow_counters_config config,
             const std::vector<sink_result::flow_shard>& src)
{
    auto sum = make_counters(config);
    auto tmp = make_counters(config);

    std::for_each(std::begin(src), std::end(src), [&](const auto& shard) {
        auto guard = utils::recycle::guard(shard.first, 0);
        std::for_each(std::begin(shard.second),
                      std::end(shard.second),
                      [&](const auto& pair) {
                          copy_flow_counters(pair.second, tmp);
                          add_flow_counters(tmp, sum);
                      });
    });

    return (sum);
}

static void to_swagger(const core::uuid& result_id,
                       const std::vector<sink_result::flow_shard>& src,
                       std::vector<std::string>& dst)
{
    uint16_t idx = 0;
    std::for_each(std::begin(src), std::end(src), [&](const auto& shard) {
        auto guard = utils::recycle::guard(shard.first, 0);
        std::for_each(
            std::begin(shard.second),
            std::end(shard.second),
            [&](const auto& pair) {
                dst.emplace_back(core::to_string(rx_flow_id(
                    result_id, idx, pair.first.first, pair.first.second)));
            });
        idx++;
    });
}

template <typename T> struct is_duration : std::false_type
{};

template <typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type
{};

template <typename SummaryTuple>
std::enable_if_t<
    is_duration<typename SummaryTuple::pop_t>::value,
    std::shared_ptr<swagger::v1::model::PacketAnalyzerFlowSummaryCounters>>
to_swagger(const SummaryTuple& src, statistics::flow::stat_t count)
{
    auto dst = std::make_shared<
        swagger::v1::model::PacketAnalyzerFlowSummaryCounters>();

    dst->setTotal(src.total.count());
    if (src.total.count()) {
        dst->setMin(
            std::chrono::duration_cast<std::chrono::nanoseconds>(src.min)
                .count());
        dst->setMax(
            std::chrono::duration_cast<std::chrono::nanoseconds>(src.max)
                .count());
        if (src.min.count() != src.max.count()) {
            dst->setStdDev(std::sqrt(
                std::chrono::duration_cast<std::chrono::nanoseconds>(src.m2)
                    .count()
                / (count - 1)));
        }
    }

    return (dst);
}

template <typename SummaryTuple>
std::enable_if_t<
    std::is_arithmetic_v<typename SummaryTuple::pop_t>,
    std::shared_ptr<swagger::v1::model::PacketAnalyzerFlowSummaryCounters>>
to_swagger(const SummaryTuple& src, statistics::flow::stat_t count)
{
    auto dst = std::make_shared<
        swagger::v1::model::PacketAnalyzerFlowSummaryCounters>();

    dst->setTotal(src.total);
    if (src.total) {
        dst->setMin(src.min);
        dst->setMax(src.max);
        if (src.min != src.max) {
            dst->setStdDev(std::sqrt(src.m2 / (count - 1)));
        }
    }

    return (dst);
}

template <typename Clock>
std::string to_rfc3339(std::chrono::time_point<Clock> from)
{
    using namespace std::chrono_literals;
    static constexpr auto ns_per_sec = std::chrono::nanoseconds(1s).count();

    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        from.time_since_epoch())
                        .count();

    time_t sec = total_ns / ns_per_sec;
    auto ns = total_ns % ns_per_sec;

    std::stringstream os;
    os << std::put_time(gmtime(&sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(9) << ns << "Z";
    return (os.str());
}

static std::shared_ptr<swagger::v1::model::PacketAnalyzerFlowHeader>
to_swagger(const statistics::flow::header_view::header_variant& variant)
{
    auto dst = std::make_shared<swagger::v1::model::PacketAnalyzerFlowHeader>();

    /*
     * Translate data to headers
     * Note: we strip out length and checksum, as those are packet specific.
     */
    std::visit(
        utils::overloaded_visitor(
            [&](const libpacket::protocol::ethernet* eth) {
                dst->setEthernet(protocol::transmogrify::to_swagger(*eth));
            },
            [&](const libpacket::protocol::ipv4* ipv4) {
                auto&& tmp = protocol::transmogrify::to_swagger(*ipv4);
                tmp->unsetChecksum();
                tmp->unsetTotal_length();
                dst->setIpv4(std::move(tmp));
            },
            [&](const libpacket::protocol::ipv6* ipv6) {
                auto&& tmp = protocol::transmogrify::to_swagger(*ipv6);
                tmp->unsetPayload_length();
                dst->setIpv6(std::move(tmp));
            },
            [&](const libpacket::protocol::mpls* mpls) {
                dst->setMpls(protocol::transmogrify::to_swagger(*mpls));
            },
            [&](const libpacket::protocol::tcp* tcp) {
                auto&& tmp = protocol::transmogrify::to_swagger(*tcp);
                tmp->unsetChecksum();
                dst->setTcp(std::move(tmp));
            },
            [&](const libpacket::protocol::udp* udp) {
                auto&& tmp = protocol::transmogrify::to_swagger(*udp);
                tmp->unsetChecksum();
                tmp->unsetLength();
                dst->setUdp(std::move(tmp));
            },
            [&](const libpacket::protocol::vlan* vlan) {
                dst->setVlan(protocol::transmogrify::to_swagger(*vlan));
            },
            [&](const std::basic_string_view<uint8_t>& sv) {
                auto data = std::string{};
                bn::encode_b64(
                    std::begin(sv), std::end(sv), std::back_inserter(data));
                dst->setUnknown(data);
            }),
        variant);

    return (dst);
}

static void populate_counters(
    const statistics::generic_flow_counters& src,
    std::shared_ptr<swagger::v1::model::PacketAnalyzerFlowCounters>& dst)
{
    using namespace openperf::packet::analyzer::statistics;

    static const std::string ns = "nanoseconds";
    static const std::string octets = "octets";

    const auto& frame_count = src.get<flow::frame_counter>();
    dst->setFrameCount(frame_count.count);
    if (frame_count.count) {
        dst->setTimestampFirst(to_rfc3339(frame_count.first_));
        dst->setTimestampLast(to_rfc3339(frame_count.last_));
    }

    if (src.holds<flow::errors>()) {
        const auto& s_src = src.get<flow::errors>();
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_errors>();

        s_dst->setFcs(s_src.fcs);
        s_dst->setIpv4Checksum(s_src.ipv4_checksum);
        s_dst->setTcpChecksum(s_src.tcp_checksum);
        s_dst->setUdpChecksum(s_src.udp_checksum);
    }
    if (src.holds<flow::sequencing>()) {
        const auto& s_src = src.get<flow::sequencing>();
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_sequence>();

        s_dst->setDropped(s_src.dropped);
        s_dst->setDuplicate(s_src.duplicate);
        s_dst->setLate(s_src.late);
        s_dst->setReordered(s_src.reordered);
        s_dst->setInOrder(s_src.in_order);
        s_dst->setRunLength(s_src.run_length);

        dst->setSequence(s_dst);
    }
    if (src.holds<flow::interarrival>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_interarrival>();

        s_dst->setSummary(
            to_swagger(src.get<flow::interarrival>(), frame_count.count - 1));
        s_dst->setUnits(ns);

        dst->setInterarrival(s_dst);
    }
    if (src.holds<flow::frame_length>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_frame_length>();

        s_dst->setSummary(
            to_swagger(src.get<flow::frame_length>(), frame_count.count));
        s_dst->setUnits(octets);

        dst->setFrameLength(s_dst);
    }
    if (src.holds<flow::jitter_ipdv>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_jitter_ipdv>();

        s_dst->setSummary(
            to_swagger(src.get<flow::jitter_ipdv>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setJitterIpdv(s_dst);
    }
    if (src.holds<flow::jitter_rfc>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_jitter_rfc>();

        s_dst->setSummary(
            to_swagger(src.get<flow::jitter_rfc>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setJitterRfc(s_dst);
    }
    if (src.holds<flow::latency>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_latency>();

        s_dst->setSummary(
            to_swagger(src.get<flow::latency>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setLatency(s_dst);
    }
    if (src.holds<flow::header>()) {
        auto view = flow::header_view(src.get<flow::header>());
        auto&& s_dst = dst->getHeaders();

        std::transform(std::begin(view),
                       std::end(view),
                       std::back_inserter(s_dst),
                       [](const auto& item) { return (to_swagger(item)); });
    }
}

analyzer_result_ptr to_swagger(const core::uuid& id, const sink_result& src)
{
    auto dst = std::make_unique<swagger::v1::model::PacketAnalyzerResult>();

    dst->setId(core::to_string(id));
    dst->setAnalyzerId(src.parent().id());
    dst->setActive(src.active());

    auto protocol_counters =
        std::make_shared<swagger::v1::model::PacketAnalyzerProtocolCounters>();
    packet::statistics::api::populate_counters(
        sum_counters(src.parent().protocol_counters(), src.protocols()),
        protocol_counters);
    dst->setProtocolCounters(protocol_counters);

    auto flow_counters =
        std::make_shared<swagger::v1::model::PacketAnalyzerFlowCounters>();
    populate_counters(sum_counters(src.parent().flow_counters(), src.flows()),
                      flow_counters);

    /* XXX: always remove headers from the top level result */
    flow_counters->getHeaders().clear();

    dst->setFlowCounters(flow_counters);

    to_swagger(id, src.flows(), dst->getFlows());

    return (dst);
}

rx_flow_ptr to_swagger(const core::uuid& id,
                       const core::uuid& result_id,
                       const statistics::generic_flow_counters& counters)
{
    auto dst = std::make_unique<swagger::v1::model::RxFlow>();

    dst->setId(core::to_string(id));
    dst->setAnalyzerResultId(core::to_string(result_id));
    auto flow_counters =
        std::make_shared<swagger::v1::model::PacketAnalyzerFlowCounters>();
    populate_counters(counters, flow_counters);
    dst->setCounters(flow_counters);

    return (dst);
}

core::uuid get_analyzer_result_id()
{
    auto id = core::uuid::random();
    std::fill_n(id.data() + 6, 10, 0);
    return (id);
}

static constexpr auto signature_stream_id_sentinel =
    std::numeric_limits<uint32_t>::max();

core::uuid rx_flow_id(const core::uuid& result_id,
                      uint16_t shard_idx,
                      uint32_t rss_hash,
                      std::optional<uint32_t> signature_stream_id)
{
    assert(shard_idx < 1024);

    auto stream_id = signature_stream_id.value_or(signature_stream_id_sentinel);

    auto tmp =
        std::array<uint8_t, 16>{result_id[0],
                                result_id[1],
                                result_id[2],
                                result_id[3],
                                result_id[4],
                                result_id[5],
                                static_cast<uint8_t>((shard_idx >> 6) | 0x40),
                                static_cast<uint8_t>((rss_hash >> 24) & 0xff),
                                static_cast<uint8_t>((shard_idx & 0x3f) | 0x40),
                                static_cast<uint8_t>((rss_hash >> 16) & 0xff),
                                static_cast<uint8_t>((rss_hash >> 8) & 0xff),
                                static_cast<uint8_t>(rss_hash & 0xff),
                                static_cast<uint8_t>((stream_id >> 24) & 0xff),
                                static_cast<uint8_t>((stream_id >> 16) & 0xff),
                                static_cast<uint8_t>((stream_id >> 8) & 0xff),
                                static_cast<uint8_t>(stream_id & 0xff)};

    return (core::uuid(tmp.data()));
}

std::tuple<core::uuid, uint16_t, uint32_t, std::optional<uint32_t>>
rx_flow_tuple(const core::uuid& id)
{
    auto min_id = core::uuid{};
    std::copy_n(id.data(), 6, min_id.data());

    uint16_t shard_idx = (id[6] & 0x0f) << 6 | (id[8] & 0x3f);
    uint32_t rss_hash = id[7] << 24 | id[9] << 16 | id[10] << 8 | id[11];
    uint32_t stream_id = id[12] << 24 | id[13] << 16 | id[14] << 8 | id[15];

    return {min_id,
            shard_idx,
            rss_hash,
            stream_id == signature_stream_id_sentinel
                ? std::nullopt
                : std::make_optional<uint32_t>(stream_id)};
}

} // namespace openperf::packet::analyzer::api
