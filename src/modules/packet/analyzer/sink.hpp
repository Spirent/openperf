#ifndef _OP_ANALYZER_SINK_HPP_
#define _OP_ANALYZER_SINK_HPP_

#include <optional>
#include <vector>

#include "core/op_core.h"
#include "packet/analyzer/api.hpp"
#include "packet/analyzer/statistics/flow/map.hpp"
#include "packet/analyzer/statistics/generic_flow_counters.hpp"
#include "packet/analyzer/statistics/generic_flow_digests.hpp"
#include "packet/statistics/generic_protocol_counters.hpp"
#include "packetio/generic_sink.hpp"
#include "utils/flat_memoize.hpp"
#include "utils/recycle.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packet::bpf {
class bpf;
}

namespace openperf::packetio::packets {
struct packet_buffer;
}

namespace openperf::packet::analyzer {

struct sink_config
{
    std::string id = core::to_string(core::uuid::random());
    std::string source;
    std::string filter;
    api::protocol_counter_flags protocol_counters =
        (packet::statistics::protocol_flags::ethernet
         | packet::statistics::protocol_flags::ip
         | packet::statistics::protocol_flags::transport);
    api::flow_counter_flags flow_counters =
        statistics::flow_counter_flags::frame_count;
    api::flow_digest_flags flow_digests = statistics::flow_digest_flags::none;
};

class sink_result
{
public:
    using recycler = utils::recycle::depot<1>;

    using flow_counters_container =
        statistics::flow::map<statistics::generic_flow_counters>;
    using flow_shard = std::pair<recycler, flow_counters_container>;

    using protocol_shard = packet::statistics::generic_protocol_counters;

    sink_result(const sink& parent);

    bool active() const;
    const sink& parent() const;

    protocol_shard& protocol(size_t idx);
    const std::vector<protocol_shard>& protocols() const;

    flow_shard& flow(size_t idx);
    const std::vector<flow_shard>& flows() const;

    void start();
    void stop();

private:
    const sink& m_parent;
    std::vector<protocol_shard> m_protocol_shards;
    std::vector<flow_shard> m_flow_shards;
    bool m_active = false;
};

class sink
{
public:
    sink(const sink_config& config, std::vector<unsigned> rx_ids);
    ~sink() = default;

    sink(sink&& other) noexcept;
    sink& operator=(sink&& other) noexcept;

    std::string id() const;
    std::string source() const;

    const sink_config& get_config() const { return m_config; }

    size_t worker_count() const;
    api::protocol_counter_flags protocol_counters() const;
    api::flow_counter_flags flow_counters() const;
    api::flow_digest_flags flow_digests() const;

    sink_result* reset(sink_result* results);
    void start(sink_result* results);
    void stop();

    bool active() const;

    bool uses_feature(packetio::packet::sink_feature_flags flags) const;

    uint16_t push(const packetio::packet::packet_buffer* const packets[],
                  uint16_t count) const;

private:
    static std::vector<uint8_t> make_indexes(std::vector<unsigned>& ids);

    uint16_t push_all(sink_result& results,
                      uint8_t index,
                      const packetio::packet::packet_buffer* const packets[],
                      uint16_t packets_length) const;

    uint16_t
    push_filtered(sink_result& results,
                  uint8_t index,
                  const packetio::packet::packet_buffer* const packets[],
                  uint16_t packets_length) const;

    sink_config m_config;
    std::vector<uint8_t> m_indexes;
    std::unique_ptr<openperf::packet::bpf::bpf> m_filter;

    mutable std::atomic<sink_result*> m_results = nullptr;
};

} // namespace openperf::packet::analyzer

#endif /* _OP_ANALYZER_SINK_HPP_ */
