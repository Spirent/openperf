#include <iomanip>
#include <sstream>

#include "packet/generator/api.hpp"
#include "packet/generator/source.hpp"
#include "packet/generator/traffic/length.hpp"

#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"
#include "swagger/v1/model/TxFlow.h"

namespace openperf::packet::generator::api {

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

static std::chrono::nanoseconds get_traffic_duration(
    const std::shared_ptr<swagger::v1::model::TrafficDuration_time>& time)
{
    auto period_type = api::to_period_type(time->getUnits());

    switch (period_type) {
    case api::period_type::hours:
        return (std::chrono::hours{time->getValue()});
    case api::period_type::milliseconds:
        return (std::chrono::milliseconds{time->getValue()});
    case api::period_type::minutes:
        return (std::chrono::minutes{time->getValue()});
    default:
        OP_LOG(OP_LOG_WARNING, "Unspecified duration units; using seconds\n");
        [[fallthrough]];
    case api::period_type::seconds:
        return (std::chrono::seconds{time->getValue()});
    }
}

static void populate_counters(
    const traffic::counter& src,
    std::shared_ptr<swagger::v1::model::PacketGeneratorFlowCounters>& dst)
{
    dst->setOctetsActual(src.octet);
    dst->setPacketsActual(src.packet);

    if (src.packet) {
        dst->setTimestampFirst(to_rfc3339(src.first_));
        dst->setTimestampLast(to_rfc3339(src.last_));
    }
}

static void populate_counters(
    const traffic::counter& src,
    std::shared_ptr<swagger::v1::model::PacketGeneratorFlowCounters>& dst,
    api::tx_rate rate,
    const traffic::sequence& sequence)
{
    dst->setOctetsActual(src.octet);
    dst->setPacketsActual(src.packet);

    if (src.packet) {
        /**
         * Use the actual recorded duration and target rate to generate
         * expected packet/octet counts.
         */
        auto exp_packets = rate * (src.last_ - src.first_);

        auto exp_octets = std::visit(
            [&](const auto& seq) {
                return (seq.sum_packet_lengths(exp_packets));
            },
            sequence);

        dst->setOctetsIntended(exp_octets);
        dst->setPacketsIntended(exp_packets);
        dst->setTimestampFirst(to_rfc3339(src.first_));
        dst->setTimestampLast(to_rfc3339(src.last_));
    }
}

generator_ptr to_swagger(const source& src)
{
    auto dst = std::make_unique<swagger::v1::model::PacketGenerator>();

    dst->setId(src.id());
    dst->setTargetId(src.target());
    dst->setActive(src.active());
    dst->setConfig(src.config());

    return (dst);
}

traffic::counter
accumulate_counters(const std::vector<traffic::counter> counters)
{
    return (std::accumulate(std::begin(counters),
                            std::end(counters),
                            traffic::counter{},
                            [](auto& lhs, const auto& rhs) {
                                auto tmp = traffic::counter{};
                                do {
                                    tmp = rhs;
                                } while (tmp.octet != rhs.octet);

                                lhs += tmp;
                                return (lhs);
                            }));
}

static void populate_remainder(
    const std::shared_ptr<swagger::v1::model::TrafficDuration>& src,
    std::shared_ptr<swagger::v1::model::DurationRemainder>& dst,
    const source_result& result,
    const traffic::counter& sum)
{
    if (src->continuousIsSet()) {
        dst->setUnit(api::to_duration_string(api::duration_type::indefinite));
    } else if (src->framesIsSet()) {
        auto tx_limit = result.parent.tx_limit().value();
        dst->setUnit(api::to_duration_string(api::duration_type::frames));
        dst->setValue(tx_limit > sum.packet ? tx_limit - sum.packet : 0);
    } else {
        assert(src->timeIsSet());

        auto run_time =
            sum.packet ? sum.last_ - sum.first_ : std::chrono::nanoseconds{0};
        auto total_time = get_traffic_duration(src->getTime());

        dst->setUnit(api::to_duration_string(api::duration_type::seconds));
        dst->setValue(total_time > run_time
                          ? std::chrono::duration_cast<std::chrono::seconds>(
                                total_time - run_time)
                                .count()
                          : 0);
    }
}

generator_result_ptr to_swagger(const core::uuid& id,
                                const source_result& result)
{
    auto dst = std::make_unique<swagger::v1::model::PacketGeneratorResult>();

    dst->setId(core::to_string(id));
    dst->setGeneratorId(result.parent.id());
    dst->setActive(result.parent.active());

    auto sum = accumulate_counters(result.counters);
    auto counters =
        std::make_shared<swagger::v1::model::PacketGeneratorFlowCounters>();
    populate_counters(
        sum, counters, result.parent.packet_rate(), result.parent.sequence());
    dst->setFlowCounters(counters);

    if (dst->isActive()) {
        auto remainder =
            std::make_shared<swagger::v1::model::DurationRemainder>();
        populate_remainder(
            result.parent.config()->getDuration(), remainder, result, sum);
        dst->setRemaining(remainder);
    }

    auto flow_idx = 0U;
    std::generate_n(
        std::back_inserter(dst->getFlows()), result.counters.size(), [&]() {
            return (core::to_string(tx_flow_id(id, flow_idx++)));
        });

    return (dst);
}

tx_flow_ptr to_swagger(const core::uuid& id,
                       const core::uuid& result_id,
                       const traffic::counter& counter)
{
    auto dst = std::make_unique<swagger::v1::model::TxFlow>();

    dst->setId(core::to_string(id));
    dst->setGeneratorResultId(core::to_string(result_id));

    auto flow_counters =
        std::make_shared<swagger::v1::model::PacketGeneratorFlowCounters>();
    populate_counters(counter, flow_counters);
    dst->setCounters(flow_counters);

    return (dst);
}

core::uuid tx_flow_id(const core::uuid& result_id, size_t flow_idx)
{
    auto tmp = std::array<uint8_t, 16>{result_id[0],
                                       result_id[1],
                                       result_id[2],
                                       result_id[3],
                                       result_id[4],
                                       result_id[5],
                                       result_id[6],
                                       result_id[7],
                                       static_cast<uint8_t>(flow_idx >> 56),
                                       static_cast<uint8_t>(flow_idx >> 48),
                                       static_cast<uint8_t>(flow_idx >> 40),
                                       static_cast<uint8_t>(flow_idx >> 32),
                                       static_cast<uint8_t>(flow_idx >> 24),
                                       static_cast<uint8_t>(flow_idx >> 16),
                                       static_cast<uint8_t>(flow_idx >> 8),
                                       static_cast<uint8_t>(flow_idx & 0xff)};

    return (core::uuid(tmp.data()));
}

std::pair<core::uuid, size_t> tx_flow_tuple(const core::uuid& id)
{
    auto min_id = core::uuid{};
    std::copy_n(id.data(), 8, min_id.data());

    size_t flow_idx =
        (static_cast<size_t>(id[8]) << 56 | static_cast<size_t>(id[9]) << 48
         | static_cast<size_t>(id[10]) << 40 | static_cast<size_t>(id[11]) << 32
         | id[12] << 24 | id[13] << 16 | id[14] << 8 | id[15]);

    return {min_id, flow_idx};
}

traffic::definition_container to_definitions(
    const std::vector<std::shared_ptr<swagger::v1::model::TrafficDefinition>>&
        api_defs)
{
    auto defs = traffic::definition_container{};
    std::transform(std::begin(api_defs),
                   std::end(api_defs),
                   std::back_inserter(defs),
                   [](const auto& traffic_def) {
                       return (std::pair(to_packet_template(*traffic_def),
                                         traffic_def->weightIsSet()
                                             ? traffic_def->getWeight()
                                             : 1));
                   });

    return (defs);
}

traffic::sequence
to_sequence(const swagger::v1::model::PacketGeneratorConfig& config)
{
    auto definitions = to_definitions(
        const_cast<swagger::v1::model::PacketGeneratorConfig&>(config)
            .getTraffic());

    switch (api::to_order_type(config.getOrder())) {
    case api::order_type::sequential:
        return (traffic::sequential_sequence(std::move(definitions)));
    default:
        OP_LOG(OP_LOG_WARNING,
               "Unspecified sequence order: using round-robin\n");
        [[fallthrough]];
    case api::order_type::round_robin:
        return (traffic::round_robin_sequence(std::move(definitions)));
    }
}

source_load to_load(const swagger::v1::model::TrafficLoad& load,
                    const traffic::sequence& sequence)
{
    using rep_type = typename packetio::packets::packets_per_hour::rep;
    using packets_per_hour =
        openperf::units::rate<rep_type, std::ratio<1, 3600>>;
    using packets_per_minute =
        openperf::units::rate<rep_type, std::ratio<1, 60>>;
    using packets_per_millisecond =
        openperf::units::rate<rep_type, std::ratio<1000>>;
    using packets_per_second = openperf::units::rate<rep_type>;

    auto period_type = api::to_period_type(load.getRate()->getPeriod());
    assert(period_type != api::period_type::none);

    auto rate = api::tx_rate{};

    switch (period_type) {
    case api::period_type::hours:
        rate = openperf::units::rate_cast<api::tx_rate>(
            packets_per_hour{load.getRate()->getValue()});
        break;
    case api::period_type::minutes:
        rate = openperf::units::rate_cast<api::tx_rate>(
            packets_per_minute{load.getRate()->getValue()});
        break;
    case api::period_type::milliseconds:
        rate = openperf::units::rate_cast<api::tx_rate>(
            packets_per_millisecond{load.getRate()->getValue()});
        break;
    default:
        OP_LOG(OP_LOG_WARNING, "Unspecified load period; using per-second\n");
        [[fallthrough]];
    case api::period_type::seconds:
        rate = openperf::units::rate_cast<api::tx_rate>(
            packets_per_second{load.getRate()->getValue()});
        break;
    }

    if (api::to_load_type(load.getUnits()) == api::load_type::octets) {
        /* Divide rate value by average frame size to get frames/hour */
        const auto length_visitor = [](const auto& seq) {
            return (seq.sum_packet_lengths());
        };
        const auto size_visitor = [](const auto& seq) { return (seq.size()); };
        rate = rate * std::visit(length_visitor, sequence)
               / std::visit(size_visitor, sequence);
    }

    using burst_type = decltype(std::declval<source_load>().burst_size);
    return {static_cast<burst_type>(load.getBurstSize()), rate};
}

std::optional<size_t>
max_transmit_count(const swagger::v1::model::TrafficDuration& duration,
                   const source_load& load)
{
    if (duration.framesIsSet()) {
        return (duration.getFrames());
    } else if (duration.timeIsSet()) {
        auto time = duration.getTime();
        return (load.rate * get_traffic_duration(time));
    } else {
        return (std::nullopt);
    }
}

} // namespace openperf::packet::generator::api