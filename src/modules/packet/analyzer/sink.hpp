#ifndef _OP_ANALYZER_SINK_HPP_
#define _OP_ANALYZER_SINK_HPP_

#include <optional>
#include <vector>

#include "core/op_core.h"
#include "packet/analyzer/api.hpp"
#include "packet/analyzer/statistics/generic_protocol_counters.hpp"
#include "packet/analyzer/statistics/generic_stream_counters.hpp"
#include "packet/analyzer/statistics/stream/counter_map.hpp"
#include "packetio/generic_sink.hpp"
#include "utils/recycle.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packetio::packets {
struct packet_buffer;
}

namespace openperf::packet::analyzer {

struct sink_config
{
    std::string id = core::to_string(core::uuid::random());
    std::string source;
    api::protocol_counters_config protocol_counters =
        (statistics::protocol_flags::ethernet | statistics::protocol_flags::ip
         | statistics::protocol_flags::protocol);
    api::stream_counters_config stream_counters =
        statistics::stream_flags::frame_count;
};

struct sink_result
{
    using recycler = utils::recycle::depot<1>;
    using stream_counters_container =
        statistics::stream::counter_map<statistics::generic_stream_counters>;

    using protocol_shard = statistics::generic_protocol_counters;
    using stream_shard = std::pair<recycler, stream_counters_container>;

    const sink& parent;

    std::vector<protocol_shard> protocol_shards;
    std::vector<stream_shard> stream_shards;

    sink_result(const sink& p);

    const std::vector<protocol_shard>& protocols() const;
    const std::vector<stream_shard>& streams() const;
};

class sink
{
public:
    sink(const sink_config& config, std::vector<unsigned> rx_ids);
    ~sink() = default;

    sink(sink&& other);
    sink& operator=(sink&& other);

    std::string id() const;
    std::string source() const;
    size_t worker_count() const;
    api::protocol_counters_config protocol_counters() const;
    api::stream_counters_config stream_counters() const;

    void start(sink_result* results);
    void stop();

    bool active() const;

    bool uses_feature(packetio::packets::sink_feature_flags flags) const;

    uint16_t
    push(const openperf::packetio::packets::packet_buffer* const packets[],
         uint16_t count) const;

private:
    static std::vector<uint8_t> make_indexes(std::vector<unsigned>& ids);

    std::string m_id;
    std::string m_source;
    api::protocol_counters_config m_protocol_counters;
    api::stream_counters_config m_stream_counters;
    std::vector<uint8_t> m_indexes;

    mutable std::atomic<sink_result*> m_results = nullptr;
    std::optional<uint64_t> m_packet_limit = std::nullopt;
};

} // namespace openperf::packet::analyzer

#endif /* _OP_ANALYZER_SINK_HPP_ */
