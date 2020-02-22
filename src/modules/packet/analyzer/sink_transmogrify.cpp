#include <iomanip>
#include <sstream>

#include "packet/analyzer/api.hpp"
#include "packet/analyzer/sink.hpp"
#include "packet/analyzer/statistics/protocol/counters.hpp"

#include "swagger/v1/model/Analyzer.h"
#include "swagger/v1/model/AnalyzerResult.h"
#include "swagger/v1/model/RxStream.h"

namespace openperf::packet::analyzer::api {

static void to_swagger(api::protocol_counters_config src,
                       std::vector<std::string>& dst)
{
    auto value = 1;
    while (value <= statistics::all_protocol_counters.value) {
        auto flag = statistics::protocol_flags{value};
        if (src & flag) { dst.emplace_back(statistics::to_name(flag)); }
        value <<= 1;
    }
}

static void to_swagger(api::stream_counters_config src,
                       std::vector<std::string>& dst)
{
    auto value = 1;
    while (value <= statistics::all_stream_counters.value) {
        auto flag = statistics::stream_flags{value};
        if (src & flag) { dst.emplace_back(statistics::to_name(flag)); }
        value <<= 1;
    }
}

analyzer_ptr to_swagger(const sink& src)
{
    auto dst = std::make_unique<swagger::v1::model::Analyzer>();

    dst->setId(src.id());
    dst->setSourceId(src.source());
    dst->setActive(src.active());

    auto config = std::make_shared<swagger::v1::model::AnalyzerConfig>();
    to_swagger(src.protocol_counters(), config->getProtocolCounters());
    to_swagger(src.stream_counters(), config->getStreamCounters());

    dst->setConfig(config);

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
    if (x.template holds<StatTuple>()) {
        auto& sum_tuple = sum.template get<StatTuple>();
        auto sum_count = sum.template get<statistics::counter>().count;
        auto& x_tuple = x.template get<StatTuple>();
        auto x_count = x.template get<statistics::counter>().count;

        /*
         * XXX: We need the totals of each tuple to calculate the variance,
         * hence we update the variance sum before adding anything else.
         */
        sum_tuple.m2 = statistics::stream::add_variance(sum_count,
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

inline statistics::generic_protocol_counters
sum_counters(protocol_counters_config config,
             const std::vector<sink_result::protocol_shard>& src)
{
    using namespace openperf::packet::analyzer::statistics;

    auto sum = make_counters(config);

    std::for_each(std::begin(src), std::end(src), [&](const auto& shard) {
        maybe_add_counter_tuple<protocol::ethernet>(shard, sum);
        maybe_add_counter_tuple<protocol::ip>(shard, sum);
        maybe_add_counter_tuple<protocol::protocol>(shard, sum);
        maybe_add_counter_tuple<protocol::tunnel>(shard, sum);
        maybe_add_counter_tuple<protocol::inner_ethernet>(shard, sum);
        maybe_add_counter_tuple<protocol::inner_ip>(shard, sum);
        maybe_add_counter_tuple<protocol::inner_protocol>(shard, sum);
    });

    return (sum);
}

inline void copy_stream_counters(const statistics::generic_stream_counters& src,
                                 statistics::generic_stream_counters& dst)
{
    using namespace openperf::packet::analyzer::statistics;

    /* basic counters are always present */
    const auto& frame_count = src.get<counter>();

    do {
        dst.get<counter>() = frame_count;
        maybe_copy_counter_tuple<stream::sequencing>(src, dst);
        maybe_copy_counter_tuple<stream::frame_length>(src, dst);
        maybe_copy_counter_tuple<stream::interarrival>(src, dst);
        maybe_copy_counter_tuple<stream::jitter_ipdv>(src, dst);
        maybe_copy_counter_tuple<stream::jitter_rfc>(src, dst);
        maybe_copy_counter_tuple<stream::latency>(src, dst);
    } while (dst.get<counter>().count != frame_count.count);
}

inline void add_stream_counters(const statistics::generic_stream_counters& x,
                                statistics::generic_stream_counters& sum)
{
    using namespace openperf::packet::analyzer::statistics;

    sum.get<counter>() += x.get<counter>();

    if (x.holds<stream::sequencing>()) {
        auto& sum_seq = sum.get<stream::sequencing>();
        const auto& x_seq = x.get<stream::sequencing>();

        sum_seq.dropped += x_seq.dropped;
        sum_seq.duplicate += x_seq.duplicate;
        sum_seq.late += x_seq.late;
        sum_seq.reordered += x_seq.reordered;
        sum_seq.in_order += x_seq.in_order;
    }

    maybe_add_summary_tuple<stream::frame_length>(x, sum);
    maybe_add_summary_tuple<stream::interarrival>(x, sum);
    maybe_add_summary_tuple<stream::jitter_ipdv>(x, sum);
    maybe_add_summary_tuple<stream::jitter_rfc>(x, sum);
    maybe_add_summary_tuple<stream::latency>(x, sum);
}

inline statistics::generic_stream_counters
sum_counters(stream_counters_config config,
             const std::vector<sink_result::stream_shard>& src)
{
    auto sum = make_counters(config);
    auto tmp = make_counters(config);

    std::for_each(std::begin(src), std::end(src), [&](const auto& shard) {
        auto guard = utils::recycle::guard(shard.first, 0);
        std::for_each(std::begin(shard.second),
                      std::end(shard.second),
                      [&](const auto& pair) {
                          copy_stream_counters(pair.second, tmp);
                          add_stream_counters(tmp, sum);
                      });
    });

    return (sum);
}

static void to_swagger(const core::uuid& result_id,
                       const std::vector<sink_result::stream_shard>& src,
                       std::vector<std::string>& dst)
{
    uint16_t idx = 0;
    std::for_each(std::begin(src), std::end(src), [&](const auto& shard) {
        auto guard = utils::recycle::guard(shard.first, 0);
        std::for_each(
            std::begin(shard.second),
            std::end(shard.second),
            [&](const auto& pair) {
                dst.emplace_back(core::to_string(rx_stream_id(
                    result_id, idx, pair.first.first, pair.first.second)));
            });
        idx++;
    });
}

static void populate_counters(
    const statistics::generic_protocol_counters& src,
    std::shared_ptr<swagger::v1::model::AnalyzerProtocolCounters>& dst)
{
    using namespace openperf::packet::analyzer::statistics;

    if (src.holds<protocol::ethernet>()) {
        const auto& p_src = src.get<protocol::ethernet>();
        auto p_dst = std::make_shared<
            swagger::v1::model::AnalyzerProtocolCounters_ethernet>();

        p_dst->setIp(p_src[protocol::ethernet::index::ether]);
        p_dst->setTimesync(p_src[protocol::ethernet::index::timesync]);
        p_dst->setArp(p_src[protocol::ethernet::index::arp]);
        p_dst->setLldp(p_src[protocol::ethernet::index::lldp]);
        p_dst->setNsh(p_src[protocol::ethernet::index::nsh]);
        p_dst->setVlan(p_src[protocol::ethernet::index::vlan]);
        p_dst->setQinq(p_src[protocol::ethernet::index::qinq]);
        p_dst->setPppoe(p_src[protocol::ethernet::index::pppoe]);
        p_dst->setFcoe(p_src[protocol::ethernet::index::fcoe]);
        p_dst->setMpls(p_src[protocol::ethernet::index::mpls]);

        dst->setEthernet(p_dst);
    }
    if (src.holds<protocol::ip>()) {
        const auto& p_src = src.get<protocol::ip>();
        auto p_dst =
            std::make_shared<swagger::v1::model::AnalyzerProtocolCounters_ip>();

        p_dst->setIpv4(p_src[protocol::ip::index::ipv4]);
        p_dst->setIpv4Ext(p_src[protocol::ip::index::ipv4_ext]);
        p_dst->setIpv4ExtUnknown(p_src[protocol::ip::index::ipv4_ext_unknown]);
        p_dst->setIpv6(p_src[protocol::ip::index::ipv6]);
        p_dst->setIpv6Ext(p_src[protocol::ip::index::ipv6_ext]);
        p_dst->setIpv6ExtUnknown(p_src[protocol::ip::index::ipv6_ext_unknown]);

        dst->setIp(p_dst);
    }
    if (src.holds<protocol::ip>()) {
        const auto& p_src = src.get<protocol::protocol>();
        auto p_dst = std::make_shared<
            swagger::v1::model::AnalyzerProtocolCounters_protocol>();

        p_dst->setTcp(p_src[protocol::protocol::index::tcp]);
        p_dst->setUdp(p_src[protocol::protocol::index::udp]);
        p_dst->setFragmented(p_src[protocol::protocol::index::fragment]);
        p_dst->setSctp(p_src[protocol::protocol::index::sctp]);
        p_dst->setIcmp(p_src[protocol::protocol::index::icmp]);
        p_dst->setNonFragmented(p_src[protocol::protocol::index::non_fragment]);
        p_dst->setIgmp(p_src[protocol::protocol::index::igmp]);

        dst->setProtocol(p_dst);
    }
    if (src.holds<protocol::tunnel>()) {
        const auto& p_src = src.get<protocol::tunnel>();
        auto p_dst = std::make_shared<
            swagger::v1::model::AnalyzerProtocolCounters_tunnel>();

        p_dst->setIp(p_src[protocol::tunnel::index::ip]);
        p_dst->setGre(p_src[protocol::tunnel::index::gre]);
        p_dst->setVxlan(p_src[protocol::tunnel::index::vxlan]);
        p_dst->setNvgre(p_src[protocol::tunnel::index::nvgre]);
        p_dst->setGeneve(p_src[protocol::tunnel::index::geneve]);
        p_dst->setGrenat(p_src[protocol::tunnel::index::grenat]);
        p_dst->setGtpc(p_src[protocol::tunnel::index::gtpc]);
        p_dst->setGtpu(p_src[protocol::tunnel::index::gtpu]);
        p_dst->setEsp(p_src[protocol::tunnel::index::esp]);
        p_dst->setL2tp(p_src[protocol::tunnel::index::l2tp]);
        p_dst->setVxlanGpe(p_src[protocol::tunnel::index::vxlan_gpe]);
        p_dst->setMplsInGre(p_src[protocol::tunnel::index::mpls_in_gre]);
        p_dst->setMplsInUdp(p_src[protocol::tunnel::index::mpls_in_udp]);

        dst->setTunnel(p_dst);
    }
    if (src.holds<protocol::inner_ethernet>()) {
        const auto& p_src = src.get<protocol::inner_ethernet>();
        auto p_dst = std::make_shared<
            swagger::v1::model::AnalyzerProtocolCounters_ethernet>();

        p_dst->setIp(p_src[protocol::inner_ethernet::index::ether]);
        p_dst->setVlan(p_src[protocol::inner_ethernet::index::vlan]);
        p_dst->setQinq(p_src[protocol::inner_ethernet::index::qinq]);

        dst->setEthernet(p_dst);
    }
    if (src.holds<protocol::inner_ip>()) {
        const auto& p_src = src.get<protocol::inner_ip>();
        auto p_dst =
            std::make_shared<swagger::v1::model::AnalyzerProtocolCounters_ip>();

        p_dst->setIpv4(p_src[protocol::inner_ip::index::ipv4]);
        p_dst->setIpv4Ext(p_src[protocol::inner_ip::index::ipv4_ext]);
        p_dst->setIpv4ExtUnknown(
            p_src[protocol::inner_ip::index::ipv4_ext_unknown]);
        p_dst->setIpv6(p_src[protocol::inner_ip::index::ipv6]);
        p_dst->setIpv6Ext(p_src[protocol::inner_ip::index::ipv6_ext]);
        p_dst->setIpv6ExtUnknown(
            p_src[protocol::inner_ip::index::ipv6_ext_unknown]);

        dst->setIp(p_dst);
    }
    if (src.holds<protocol::inner_ip>()) {
        const auto& p_src = src.get<protocol::inner_protocol>();
        auto p_dst = std::make_shared<
            swagger::v1::model::AnalyzerProtocolCounters_protocol>();

        p_dst->setTcp(p_src[protocol::inner_protocol::index::tcp]);
        p_dst->setUdp(p_src[protocol::inner_protocol::index::udp]);
        p_dst->setFragmented(p_src[protocol::inner_protocol::index::fragment]);
        p_dst->setSctp(p_src[protocol::inner_protocol::index::sctp]);
        p_dst->setIcmp(p_src[protocol::inner_protocol::index::icmp]);
        p_dst->setNonFragmented(
            p_src[protocol::inner_protocol::index::non_fragment]);

        dst->setProtocol(p_dst);
    }
}

template <typename T> struct is_duration : std::false_type
{};

template <typename Rep, typename Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type
{};

template <typename SummaryTuple>
std::enable_if_t<
    is_duration<typename SummaryTuple::pop_t>::value,
    std::shared_ptr<swagger::v1::model::AnalyzerStreamSummaryCounters>>
to_swagger(const SummaryTuple& src, statistics::stat_t count)
{
    auto dst =
        std::make_shared<swagger::v1::model::AnalyzerStreamSummaryCounters>();

    dst->setTotal(src.total.count());
    if (src.total.count()) {
        dst->setMin(src.min.count());
        dst->setMax(src.max.count());
        if (src.min.count() != src.max.count()) {
            dst->setStdDev(std::sqrt(src.m2.count() / (count - 1)));
        }
    }

    return (dst);
}

template <typename SummaryTuple>
std::enable_if_t<
    std::is_arithmetic_v<typename SummaryTuple::pop_t>,
    std::shared_ptr<swagger::v1::model::AnalyzerStreamSummaryCounters>>
to_swagger(const SummaryTuple& src, statistics::stat_t count)
{
    auto dst =
        std::make_shared<swagger::v1::model::AnalyzerStreamSummaryCounters>();

    dst->setTotal(src.total);
    if (src.total) {
        dst->setMin(src.min);
        dst->setMax(src.max);
        if (src.min != src.max) {
            dst->setStdDev(std::sqrt(src.m2 / count - 1));
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

static void populate_counters(
    const statistics::generic_stream_counters& src,
    std::shared_ptr<swagger::v1::model::AnalyzerStreamCounters>& dst)
{
    using namespace openperf::packet::analyzer::statistics;

    static const std::string ns = "nanoseconds";
    static const std::string octets = "octets";

    const auto& frame_count = src.get<counter>();
    dst->setFrameCount(frame_count.count);
    if (frame_count.count) {
        dst->setTimestampFirst(to_rfc3339(frame_count.first_));
        dst->setTimestampLast(to_rfc3339(frame_count.last_));
    }

    if (src.holds<stream::sequencing>()) {
        const auto& s_src = src.get<stream::sequencing>();
        auto s_dst = std::make_shared<
            swagger::v1::model::AnalyzerStreamCounters_sequence>();

        s_dst->setDropped(s_src.dropped);
        s_dst->setDuplicate(s_src.duplicate);
        s_dst->setLate(s_src.late);
        s_dst->setReordered(s_src.reordered);
        s_dst->setInOrder(s_src.in_order);
        s_dst->setRunLength(s_src.run_length);

        dst->setSequence(s_dst);
    }
    if (src.holds<stream::interarrival>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::AnalyzerStreamCounters_interarrival>();

        s_dst->setSummary(
            to_swagger(src.get<stream::interarrival>(), frame_count.count - 1));
        s_dst->setUnits(ns);

        dst->setInterarrival(s_dst);
    }
    if (src.holds<stream::frame_length>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::AnalyzerStreamCounters_frame_length>();

        s_dst->setSummary(
            to_swagger(src.get<stream::frame_length>(), frame_count.count));
        s_dst->setUnits(octets);

        dst->setFrameLength(s_dst);
    }
    if (src.holds<stream::jitter_ipdv>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::AnalyzerStreamCounters_jitter_ipdv>();

        s_dst->setSummary(
            to_swagger(src.get<stream::jitter_ipdv>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setJitterIpdv(s_dst);
    }
    if (src.holds<stream::jitter_rfc>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::AnalyzerStreamCounters_jitter_rfc>();

        s_dst->setSummary(
            to_swagger(src.get<stream::jitter_rfc>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setJitterRfc(s_dst);
    }
    if (src.holds<stream::latency>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::AnalyzerStreamCounters_latency>();

        s_dst->setSummary(
            to_swagger(src.get<stream::latency>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setLatency(s_dst);
    }
}

analyzer_result_ptr to_swagger(const core::uuid& id, const sink_result& src)
{
    auto dst = std::make_unique<swagger::v1::model::AnalyzerResult>();

    dst->setId(core::to_string(id));
    dst->setAnalyzerId(src.parent.id());
    dst->setActive(src.parent.active());

    auto protocol_counters =
        std::make_shared<swagger::v1::model::AnalyzerProtocolCounters>();
    populate_counters(
        sum_counters(src.parent.protocol_counters(), src.protocols()),
        protocol_counters);
    dst->setProtocolCounters(protocol_counters);

    auto stream_counters =
        std::make_shared<swagger::v1::model::AnalyzerStreamCounters>();
    populate_counters(sum_counters(src.parent.stream_counters(), src.streams()),
                      stream_counters);
    dst->setStreamCounters(stream_counters);

    to_swagger(id, src.streams(), dst->getStreams());

    return (dst);
}

rx_stream_ptr to_swagger(const core::uuid& id,
                         const core::uuid& result_id,
                         const statistics::generic_stream_counters& counters)
{
    auto dst = std::make_unique<swagger::v1::model::RxStream>();

    dst->setId(core::to_string(id));
    dst->setAnalyzerResultId(core::to_string(result_id));
    auto stream_counters =
        std::make_shared<swagger::v1::model::AnalyzerStreamCounters>();
    populate_counters(counters, stream_counters);
    dst->setCounters(stream_counters);

    return (dst);
}

static constexpr auto signature_stream_id_sentinel =
    std::numeric_limits<uint32_t>::max();

core::uuid rx_stream_id(const core::uuid& result_id,
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
rx_stream_tuple(const core::uuid& id)
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
