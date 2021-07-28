#ifndef _OP_ANALYZER_STATISTICS_FLOW_UPDATE_HPP_
#define _OP_ANALYZER_STATISTICS_FLOW_UPDATE_HPP_

#include "packet/analyzer/statistics/generic_flow_digests.hpp"
#include "packet/analyzer/statistics/flow/counters.hpp"
#include "packet/statistics/tuple_utils.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::analyzer::statistics::flow {

/**
 * Update function for arbitrary collection of statistics in a tuple.
 **/

template <typename StatsTuple>
void update(StatsTuple& tuple, const packetio::packet::packet_buffer* pkt)
{
    using namespace openperf::packet::statistics;

    static_assert(has_type<counter::frame_counter, StatsTuple>::value);

    if constexpr (has_type_v<generic_flow_digests, StatsTuple>) {
        get_counter<generic_flow_digests>(tuple).write_prefetch();
    }

    const auto rx = packetio::packet::rx_timestamp(pkt);

    auto& frames = get_counter<counter::frame_counter>(tuple);
    auto last_rx = frames.last();
    update(frames, rx, pkt);
    if constexpr (has_type_v<counter::interarrival, StatsTuple>) {
        if (last_rx) {
            auto ia = rx - *last_rx;
            update(get_counter<counter::interarrival>(tuple),
                   ia,
                   frames.count - 1);

            if constexpr (has_type_v<generic_flow_digests, StatsTuple>) {
                auto& digests = get_counter<generic_flow_digests>(tuple);
                digests.template update<digest::interarrival>(ia);
            }
        }
    }

    if constexpr (has_type_v<counter::frame_length, StatsTuple>) {
        auto frame_length = packetio::packet::frame_length(pkt);
        update(get_counter<counter::frame_length>(tuple),
               frame_length,
               frames.count);

        if constexpr (has_type_v<generic_flow_digests, StatsTuple>) {
            auto& digests = get_counter<generic_flow_digests>(tuple);
            digests.template update<digest::frame_length>(frame_length);
        }
    }

    if constexpr (has_type_v<counter::sequencing, StatsTuple>) {
        if (const auto seq_num =
                packetio::packet::signature_sequence_number(pkt)) {
            auto& seq_stats = get_counter<counter::sequencing>(tuple);
            const auto prev_run_length = seq_stats.run_length;

            update(seq_stats, *seq_num, 1000);

            if constexpr (has_type_v<generic_flow_digests, StatsTuple>) {
                if (seq_stats.run_length == 1) { /* this packet is oos */
                    auto& digests = get_counter<generic_flow_digests>(tuple);
                    digests.template update<digest::sequence_run_length>(
                        prev_run_length);
                }
            }
        }
    }

    if constexpr (has_type_v<counter::latency, StatsTuple>) {
        if (const auto tx = packetio::packet::signature_tx_timestamp(pkt)) {
            auto& latency_stats = get_counter<counter::latency>(tuple);
            auto last_delay = latency_stats.last();
            auto delay = rx - *tx;
            update(latency_stats, delay, frames.count);

            if constexpr (has_type_v<generic_flow_digests, StatsTuple>) {
                auto& digests = get_counter<generic_flow_digests>(tuple);
                digests.template update<digest::latency>(delay);
            }

            if (last_delay) {
                if constexpr (has_type_v<counter::jitter_rfc, StatsTuple>) {
                    auto jitter = std::chrono::abs(delay - *last_delay);
                    update(get_counter<counter::jitter_rfc>(tuple),
                           jitter,
                           frames.count);

                    if constexpr (has_type_v<generic_flow_digests,
                                             StatsTuple>) {
                        auto& digests =
                            get_counter<generic_flow_digests>(tuple);
                        digests.template update<digest::jitter_rfc>(jitter);
                    }
                }

                if constexpr (has_type_v<counter::jitter_ipdv, StatsTuple>) {
                    static_assert(has_type_v<counter::sequencing, StatsTuple>);
                    if (get_counter<counter::sequencing>(tuple).run_length
                        > 1) {
                        /* frame is in sequence */
                        auto jitter = std::chrono::duration_cast<
                            counter::jitter_ipdv::pop_t>(delay - *last_delay);
                        update(get_counter<counter::jitter_ipdv>(tuple),
                               jitter,
                               frames.count);

                        if constexpr (has_type_v<generic_flow_digests,
                                                 StatsTuple>) {
                            auto& digests =
                                get_counter<generic_flow_digests>(tuple);
                            digests.template update<digest::jitter_ipdv>(
                                jitter);
                        }
                    }
                }
            }
        }
    }

    if constexpr (has_type_v<counter::prbs, StatsTuple>) {
        update(get_counter<counter::prbs>(tuple), pkt);
    }
}

} // namespace openperf::packet::analyzer::statistics::flow

#endif /* _OP_ANALYZER_STATISTICS_FLOW_UPDATE_HPP_ */
