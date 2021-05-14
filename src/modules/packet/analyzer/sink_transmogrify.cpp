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

static std::vector<std::string> to_swagger(api::protocol_counter_flags flags)
{
    auto counters = std::vector<std::string>{};
    auto value = 1;
    while (value <= packet::statistics::all_protocol_counters.value) {
        auto flag = packet::statistics::protocol_flags{value};
        if (flags & flag) {
            counters.emplace_back(packet::statistics::to_name(flag));
        }
        value <<= 1;
    }

    return (counters);
}

static std::vector<std::string> to_swagger(api::flow_counter_flags flags)
{
    auto counters = std::vector<std::string>{};
    auto value = 1;
    while (value <= statistics::all_flow_counters.value) {
        auto flag = statistics::flow_counter_flags{value};
        if (flags & flag) { counters.emplace_back(statistics::to_name(flag)); }
        value <<= 1;
    }

    return (counters);
}

static std::vector<std::string> to_swagger(api::flow_digest_flags flags)
{
    auto digests = std::vector<std::string>{};
    auto value = 1;
    while (value <= statistics::all_flow_digests.value) {
        auto flag = statistics::flow_digest_flags{value};
        if (flags & flag) { digests.emplace_back(statistics::to_name(flag)); }
        value <<= 1;
    }

    return (digests);
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
    dst_config->getProtocolCounters() =
        to_swagger(src_config.protocol_counters);
    dst_config->getFlowCounters() = to_swagger(src_config.flow_counters);
    dst_config->getFlowDigests() = to_swagger(src_config.flow_digests);
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
    using namespace statistics::flow::counter;

    if (x.template holds<StatTuple>()) {
        auto& sum_tuple = sum.template get<StatTuple>();
        auto sum_count = sum.template get<frame_counter>().count;
        auto& x_tuple = x.template get<StatTuple>();
        auto x_count = x.template get<frame_counter>().count;

        /*
         * XXX: We need the totals of each tuple to calculate the variance,
         * hence we update the variance sum before adding anything else.
         */
        sum_tuple.m2 = add_variance(sum_count,
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

template <typename Digest>
void maybe_add_flow_digest(statistics::generic_flow_digests& lhs,
                           const statistics::generic_flow_digests& rhs)
{
    if (lhs.holds<Digest>() && rhs.holds<Digest>()) {
        auto& ld = lhs.get<Digest>();
        const auto& rd = rhs.get<Digest>();
        ld += rd;
    }
}

inline void add_flow_digests(statistics::generic_flow_digests& rhs,
                             const statistics::generic_flow_digests& lhs)
{
    using namespace openperf::packet::analyzer::statistics::flow::digest;

    maybe_add_flow_digest<frame_length>(rhs, lhs);
    maybe_add_flow_digest<interarrival>(rhs, lhs);
    maybe_add_flow_digest<jitter_ipdv>(rhs, lhs);
    maybe_add_flow_digest<jitter_rfc>(rhs, lhs);
    maybe_add_flow_digest<latency>(rhs, lhs);
    maybe_add_flow_digest<sequence_run_length>(rhs, lhs);
}

inline packet::statistics::generic_protocol_counters
sum_protocol_counters(protocol_counter_flags flags,
                      const std::vector<sink_result::protocol_shard>& src)
{
    using namespace openperf::packet::statistics::protocol;

    auto sum = make_counters(flags);

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
    using namespace openperf::packet::analyzer::statistics;

    /* basic counters are always present */
    const auto& src_frames = src.get<flow::counter::frame_counter>();

    do {
        dst.get<flow::counter::frame_counter>() = src_frames;
        maybe_copy_counter_tuple<flow::counter::frame_length>(src, dst);
        maybe_copy_counter_tuple<flow::counter::interarrival>(src, dst);
        maybe_copy_counter_tuple<flow::counter::jitter_ipdv>(src, dst);
        maybe_copy_counter_tuple<flow::counter::jitter_rfc>(src, dst);
        maybe_copy_counter_tuple<flow::counter::latency>(src, dst);
        maybe_copy_counter_tuple<flow::counter::prbs>(src, dst);
        maybe_copy_counter_tuple<flow::counter::sequencing>(src, dst);
        maybe_copy_counter_tuple<generic_flow_digests>(src, dst);
    } while (dst.get<flow::counter::frame_counter>().count != src_frames.count);

    maybe_copy_counter_tuple<flow::header>(src, dst);
}

inline void add_flow_counters(const statistics::generic_flow_counters& x,
                              statistics::generic_flow_counters& sum)
{
    using namespace openperf::packet::analyzer::statistics::flow::counter;

    if (x.holds<prbs>()) {
        auto& sum_prbs = sum.get<prbs>();
        const auto& x_prbs = x.get<prbs>();

        sum_prbs.bit_errors += x_prbs.bit_errors;
        sum_prbs.frame_errors += x_prbs.frame_errors;
        sum_prbs.octets += x_prbs.octets;
    }

    if (x.holds<sequencing>()) {
        auto& sum_seq = sum.get<sequencing>();
        const auto& x_seq = x.get<sequencing>();

        sum_seq.dropped += x_seq.dropped;
        sum_seq.duplicate += x_seq.duplicate;
        sum_seq.late += x_seq.late;
        sum_seq.reordered += x_seq.reordered;
        sum_seq.in_order += x_seq.in_order;
    }

    maybe_add_summary_tuple<frame_length>(x, sum);
    maybe_add_summary_tuple<interarrival>(x, sum);
    maybe_add_summary_tuple<jitter_ipdv>(x, sum);
    maybe_add_summary_tuple<jitter_rfc>(x, sum);
    maybe_add_summary_tuple<latency>(x, sum);

    if (x.holds<statistics::generic_flow_digests>()) {
        auto& sum_digest = sum.get<statistics::generic_flow_digests>();
        const auto& x_digest = x.get<statistics::generic_flow_digests>();
        add_flow_digests(sum_digest, x_digest);
    }

    /*
     * XXX: Sum the counter structure last.  "Adding" the summary tuples
     * together requires the frame count to be correct for each tuple.
     */
    sum.get<frame_counter>() += x.get<frame_counter>();
}

inline statistics::generic_flow_counters
sum_flow_counters(flow_counter_flags counter_flags,
                  flow_digest_flags digest_flags,
                  const std::vector<sink_result::flow_shard>& src)
{
    auto sum = make_flow_counters(counter_flags, digest_flags);
    auto tmp = make_flow_counters(counter_flags, digest_flags);

    std::for_each(std::begin(src), std::end(src), [&](const auto& shard) {
        auto guard = utils::recycle::guard(shard.first, api::result_reader_id);
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
        auto guard = utils::recycle::guard(shard.first, api::result_reader_id);
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
to_swagger(const SummaryTuple& src, statistics::flow::counter::stat_t count)
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
to_swagger(const SummaryTuple& src, statistics::flow::counter::stat_t count)
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

static void populate_flow_counters(
    const statistics::generic_flow_counters& src,
    std::shared_ptr<swagger::v1::model::PacketAnalyzerFlowCounters>& dst)
{
    using namespace openperf::packet::analyzer::statistics::flow;

    static const std::string ns = "nanoseconds";
    static const std::string octets = "octets";

    const auto& frame_count = src.get<counter::frame_counter>();
    dst->setFrameCount(frame_count.count);
    if (frame_count.count) {
        dst->setTimestampFirst(to_rfc3339(frame_count.first_));
        dst->setTimestampLast(to_rfc3339(frame_count.last_));
    }

    auto errors_dst = std::make_shared<
        swagger::v1::model::PacketAnalyzerFlowCounters_errors>();

    errors_dst->setFcs(frame_count.errors.fcs);
    errors_dst->setIpv4Checksum(frame_count.errors.ipv4_checksum);
    errors_dst->setTcpChecksum(frame_count.errors.tcp_checksum);
    errors_dst->setUdpChecksum(frame_count.errors.udp_checksum);

    dst->setErrors(errors_dst);

    if (src.holds<counter::frame_length>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_frame_length>();

        s_dst->setSummary(
            to_swagger(src.get<counter::frame_length>(), frame_count.count));
        s_dst->setUnits(octets);

        dst->setFrameLength(s_dst);
    }
    if (src.holds<header>()) {
        auto view = header_view(src.get<header>());
        auto&& s_dst = dst->getHeaders();

        std::transform(std::begin(view),
                       std::end(view),
                       std::back_inserter(s_dst),
                       [](const auto& item) { return (to_swagger(item)); });
    }
    if (src.holds<counter::interarrival>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_interarrival>();

        s_dst->setSummary(to_swagger(src.get<counter::interarrival>(),
                                     frame_count.count - 1));
        s_dst->setUnits(ns);

        dst->setInterarrival(s_dst);
    }
    if (src.holds<counter::jitter_ipdv>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_jitter_ipdv>();

        s_dst->setSummary(
            to_swagger(src.get<counter::jitter_ipdv>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setJitterIpdv(s_dst);
    }
    if (src.holds<counter::jitter_rfc>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_jitter_rfc>();

        s_dst->setSummary(
            to_swagger(src.get<counter::jitter_rfc>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setJitterRfc(s_dst);
    }
    if (src.holds<counter::latency>()) {
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_latency>();

        s_dst->setSummary(
            to_swagger(src.get<counter::latency>(), frame_count.count));
        s_dst->setUnits(ns);

        dst->setLatency(s_dst);
    }
    if (src.holds<counter::prbs>()) {
        const auto& s_src = src.get<counter::prbs>();
        auto s_dst = std::make_shared<
            swagger::v1::model::PacketAnalyzerFlowCounters_prbs>();

        s_dst->setBitErrors(s_src.bit_errors);
        s_dst->setFrameErrors(s_src.frame_errors);
        s_dst->setOctets(s_src.octets);

        dst->setPrbs(s_dst);
    }
    if (src.holds<counter::sequencing>()) {
        const auto& s_src = src.get<counter::sequencing>();
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
}

template <typename DigestType>
std::shared_ptr<swagger::v1::model::PacketAnalyzerFlowDigestResult>
to_swagger(const DigestType& digest)
{
    auto dst =
        std::make_shared<swagger::v1::model::PacketAnalyzerFlowDigestResult>();

    const auto centroids = digest.get();
    std::transform(
        std::begin(centroids),
        std::end(centroids),
        std::back_inserter(dst->getCentroids()),
        [](const auto& pair) {
            auto tmp = std::make_unique<swagger::v1::model::TDigestCentroid>();
            if constexpr (is_duration<typename DigestType::mean_type>::value) {
                tmp->setMean(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        pair.first)
                        .count());
            } else {
                tmp->setMean(pair.first);
            }
            tmp->setWeight(pair.second);
            return (tmp);
        });

    return (dst);
}

static std::shared_ptr<swagger::v1::model::PacketAnalyzerFlowDigests>
to_swagger(const statistics::generic_flow_digests& src)
{
    using namespace openperf::packet::analyzer::statistics::flow::digest;

    auto dst =
        std::make_shared<swagger::v1::model::PacketAnalyzerFlowDigests>();

    if (src.holds<frame_length>()) {
        dst->setFrameLength(to_swagger(src.get<frame_length>()));
    }

    if (src.holds<interarrival>()) {
        dst->setInterarrivalTime(to_swagger(src.get<interarrival>()));
    }

    if (src.holds<jitter_ipdv>()) {
        dst->setJitterIpdv(to_swagger(src.get<jitter_ipdv>()));
    }

    if (src.holds<jitter_rfc>()) {
        dst->setJitterRfc(to_swagger(src.get<jitter_rfc>()));
    }

    if (src.holds<latency>()) {
        dst->setLatency(to_swagger(src.get<latency>()));
    }

    if (src.holds<sequence_run_length>()) {
        dst->setSequenceRunLength(to_swagger(src.get<sequence_run_length>()));
    }

    return (dst);
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
        sum_protocol_counters(src.parent().protocol_counters(),
                              src.protocols()),
        protocol_counters);
    dst->setProtocolCounters(protocol_counters);

    auto flow_counters =
        std::make_shared<swagger::v1::model::PacketAnalyzerFlowCounters>();

    auto sum = sum_flow_counters(
        src.parent().flow_counters(), src.parent().flow_digests(), src.flows());
    populate_flow_counters(sum, flow_counters);

    /* XXX: always remove headers from the top level result */
    flow_counters->getHeaders().clear();

    dst->setFlowCounters(flow_counters);

    if (sum.holds<statistics::generic_flow_digests>()) {
        dst->setFlowDigests(
            to_swagger(sum.get<statistics::generic_flow_digests>()));
    }

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
    populate_flow_counters(counters, flow_counters);
    dst->setCounters(flow_counters);

    if (counters.holds<statistics::generic_flow_digests>()) {
        dst->setDigests(
            to_swagger(counters.get<statistics::generic_flow_digests>()));
    }

    return (dst);
}

core::uuid get_analyzer_result_id()
{
    auto id = core::uuid::random();
    std::fill_n(id.data() + 6, 10, 0);
    return (id);
}

/*
 * TestCenter doesn't like 0 for the stream id, so use that value to indicate
 * no stream id is available. We should never use a stream id value of 0 either,
 * because our flow id's are always incremented by 1. Consult the `to_stream_id`
 * function in packet/generator/traffic/sequence.cpp.
 */
static constexpr auto signature_stream_id_sentinel = 0;

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
