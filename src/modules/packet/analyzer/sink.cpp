#include <mutex>
#include <thread>

#include "packetio/internal_client.hpp"
#include "packetio/internal_worker.hpp"
#include "packetio/packet_buffer.hpp"
#include "packet/analyzer/sink.hpp"
#include "packet/analyzer/statistics/flow/counter_map.tcc"
#include "spirent_pga/api.h"
#include "utils/flat_memoize.hpp"

namespace openperf::packet::analyzer {

/* Instantiate our templatized data structures */
template class statistics::flow::counter_map<statistics::generic_flow_counters>;

static std::once_flag flow_counter_factory_init;

sink_result::sink_result(const sink& parent_)
    : parent(parent_)
    , flow_shards(parent.worker_count())
{
    assert(parent.worker_count());
    std::generate_n(
        std::back_inserter(protocol_shards), parent.worker_count(), [&]() {
            return (statistics::make_counters(parent.protocol_counters()));
        });

    /*
     * The counter factory functions don't perform their initialization until
     * called.  The protocol counter factory was initialized above, however,
     * we don't create flow counters until we receive a packet.  Hence, if
     * we haven't done so already, ask the flow factory for a dummy counter
     * now so that we can initialize it here instead of on the sink's fast path.
     */
    std::call_once(flow_counter_factory_init, []() {
        [[maybe_unused]] auto flow_counters =
            statistics::make_counters(statistics::all_flow_counters);
    });
}

const std::vector<sink_result::protocol_shard>& sink_result::protocols() const
{
    return (protocol_shards);
}

const std::vector<sink_result::flow_shard>& sink_result::flows() const
{
    return (flow_shards);
}

std::vector<uint8_t> sink::make_indexes(std::vector<unsigned>& ids)
{
    std::vector<uint8_t> indexes(*max_element(std::begin(ids), std::end(ids)));
    assert(indexes.size() < std::numeric_limits<uint8_t>::max());
    uint8_t idx = 0;
    for (auto& id : ids) { indexes[id] = idx++; }
    return (indexes);
}

sink::sink(const sink_config& config, std::vector<unsigned> rx_ids)
    : m_id(config.id)
    , m_source(config.source)
    , m_indexes(sink::make_indexes(rx_ids))
    , m_flow_counters(config.flow_counters)
    , m_protocol_counters(config.protocol_counters)
{}

sink::sink(sink&& other)
    : m_id(std::move(other.m_id))
    , m_source(std::move(other.m_source))
    , m_indexes(std::move(other.m_indexes))
    , m_flow_counters(other.m_flow_counters)
    , m_protocol_counters(other.m_protocol_counters)
    , m_results(other.m_results.load())
{}

sink& sink::operator=(sink&& other)
{
    if (this != &other) {
        m_id = std::move(other.m_id);
        m_source = std::move(other.m_source);
        m_indexes = std::move(other.m_indexes);
        m_flow_counters = other.m_flow_counters;
        m_protocol_counters = other.m_protocol_counters;
        m_results.store(other.m_results);
    }

    return (*this);
}

std::string sink::id() const { return (m_id); }

std::string sink::source() const { return (m_source); }

api::protocol_counters_config sink::protocol_counters() const
{
    return (m_protocol_counters);
}

api::flow_counters_config sink::flow_counters() const
{
    return (m_flow_counters);
}

size_t sink::worker_count() const { return (m_indexes.size()); }

void sink::start(sink_result* results)
{
    m_results.store(results, std::memory_order_release);
}

void sink::stop()
{
    /*
     * XXX: Technically, we have a race here.  Workers could still be using the
     * results pointer when we release it and the server could potentially
     * delete the result. Of course, for that to happen, the server would have
     * to respond to the client for the stop request AND process a delete
     * request before the worker finishes processing the last burst of
     * packets.  I guess the proper solution is to spin on an RCU mechanism
     * here, but that seems like overkill for something so unlikely to happen...
     */
    m_results.store(nullptr, std::memory_order_release);
}

bool sink::active() const
{
    return (m_results.load(std::memory_order_relaxed) != nullptr);
};

bool sink::uses_feature(packetio::packet::sink_feature_flags flags) const
{
    using sink_feature_flags = packetio::packet::sink_feature_flags;

    /*
     * Intelligently determine what features we need based on our
     * flow statistics configuration.
     */
    constexpr auto signature_stats =
        (statistics::flow_flags::sequencing | statistics::flow_flags::latency
         | statistics::flow_flags::jitter_ipdv
         | statistics::flow_flags::jitter_rfc);

    /* We always need rx_timestamps */
    auto needed = openperf::utils::bit_flags<sink_feature_flags>{
        sink_feature_flags::rx_timestamp};

    if (m_flow_counters & signature_stats) {
        needed |= sink_feature_flags::spirent_signature_decode;
    }

    return (bool(needed & flags));
}

uint16_t sink::push(const packetio::packet::packet_buffer* const packets[],
                    uint16_t packets_length) const
{
    static constexpr int burst_size = 64;
    const auto id = packetio::internal::worker::get_id();
    auto results = m_results.load(std::memory_order_consume);

    /* Update protocol statistics */
    auto& protocol = results->protocol_shards[m_indexes[id]];
    uint16_t start = 0;
    while (start < packets_length) {
        uint16_t end = start + std::min(burst_size, packets_length - start);
        uint16_t count = end - start;

        auto packet_types = std::array<uint32_t, burst_size>{};
        std::transform(packets + start,
                       packets + end,
                       packet_types.data(),
                       [](const auto& packet) {
                           return (packetio::packet::type(packet));
                       });

        protocol.update(packet_types.data(), count);

        start = end;
    }

    /* Update flow statistics */
    auto& flows = results->flow_shards[m_indexes[id]];
    auto cache = openperf::utils::flat_memoize<64>(
        [&](uint32_t rss_hash, std::optional<uint32_t> stream_id) {
            return (flows.second.find(rss_hash, stream_id));
        });

    std::for_each(packets, packets + packets_length, [&](const auto& packet) {
        auto hash = packetio::packet::rss_hash(packet);
        auto stream_id = packetio::packet::signature_stream_id(packet);
        auto counters = cache(hash, stream_id);
        if (!counters) { /* New flow; create stats */
            auto to_delete = flows.second.insert(
                hash, stream_id, statistics::make_counters(m_flow_counters));

            counters = cache.retry(hash, stream_id);
            assert(counters);
            flows.first.writer_add_gc_callback(
                [to_delete]() { delete to_delete; });
        }
        counters->update(packetio::packet::length(packet),
                         packetio::packet::rx_timestamp(packet),
                         packetio::packet::signature_tx_timestamp(packet),
                         packetio::packet::signature_sequence_number(packet));
    });

    flows.first.writer_process_gc_callbacks();

    return (packets_length);
}

} // namespace openperf::packet::analyzer
