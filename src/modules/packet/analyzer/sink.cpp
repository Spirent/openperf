#include <mutex>
#include <thread>

#include "packetio/internal_client.hpp"
#include "packetio/internal_worker.hpp"
#include "packetio/packet_buffer.hpp"
#include "packet/bpf/bpf.hpp"
#include "packet/bpf/bpf_sink.hpp"
#include "packet/analyzer/sink.hpp"
#include "packet/analyzer/statistics/flow/map.tcc"
#include "spirent_pga/api.h"
#include "utils/flat_memoize.hpp"

namespace openperf::packet::analyzer {

constexpr uint16_t burst_size_max = 64;

/* Instantiate our templatized data structures */
template class statistics::flow::map<statistics::generic_flow_counters>;

static std::once_flag flow_counter_factory_init;

sink_result::sink_result(const sink& parent)
    : m_parent(parent)
    , m_flow_shards(parent.worker_count())
{
    assert(parent.worker_count());
    std::generate_n(
        std::back_inserter(m_protocol_shards), m_parent.worker_count(), [&]() {
            return (packet::statistics::make_counters(
                m_parent.protocol_counters()));
        });

    /* Alert the RCU system that we will have a reader */
    std::for_each(
        std::begin(m_flow_shards), std::end(m_flow_shards), [](auto& shard) {
            shard.first.writer_add_reader(api::result_reader_id);
        });

    /*
     * The counter factory functions don't perform their initialization until
     * called.  The protocol counter factory was initialized above, however,
     * we don't create flow counters until we receive a packet.  Hence, if
     * we haven't done so already, ask the flow factory for a dummy counter
     * now so that we can initialize it here instead of on the sink's fast path.
     */
    std::call_once(flow_counter_factory_init, []() {
        [[maybe_unused]] auto flow_counters = statistics::make_flow_counters(
            statistics::all_flow_counters, statistics::all_flow_digests);
    });
}

bool sink_result::active() const { return (m_active); }

const sink& sink_result::parent() const { return (m_parent); }

sink_result::protocol_shard& sink_result::protocol(size_t idx)
{
    return (m_protocol_shards[idx]);
}

const std::vector<sink_result::protocol_shard>& sink_result::protocols() const
{
    return (m_protocol_shards);
}

sink_result::flow_shard& sink_result::flow(size_t idx)
{
    return (m_flow_shards[idx]);
}

const std::vector<sink_result::flow_shard>& sink_result::flows() const
{
    return (m_flow_shards);
}

void sink_result::start() { m_active = true; }

void sink_result::stop() { m_active = false; }

std::vector<uint8_t> sink::make_indexes(std::vector<unsigned>& ids)
{
    std::vector<uint8_t> indexes(*max_element(std::begin(ids), std::end(ids))
                                 + 1);
    assert(indexes.size() < std::numeric_limits<uint8_t>::max());
    uint8_t idx = 0;
    for (auto& id : ids) { indexes[id] = idx++; }
    return (indexes);
}

sink::sink(const sink_config& config, std::vector<unsigned> rx_ids)
    : m_config(config)
    , m_indexes(sink::make_indexes(rx_ids))
{
    if (!m_config.filter.empty()) {
        m_filter =
            std::make_unique<openperf::packet::bpf::bpf>(m_config.filter);

        auto bpf_filter_flags = m_filter->get_filter_flags();
        auto bpf_features = bpf::bpf_sink_feature_flags(bpf_filter_flags);
        OP_LOG(OP_LOG_DEBUG,
               "Analyzer BPF filter '%s' flags %#x sink_features %#x",
               m_config.filter.c_str(),
               bpf_filter_flags,
               bpf_features.value);
    }
}

sink::sink(sink&& other) noexcept
    : m_config(std::move(other.m_config))
    , m_indexes(std::move(other.m_indexes))
    , m_filter(std::move(other.m_filter))
    , m_results(other.m_results.load())
{}

sink& sink::operator=(sink&& other) noexcept
{
    if (this != &other) {
        m_config = std::move(other.m_config);
        m_indexes = std::move(other.m_indexes);
        m_filter = std::move(other.m_filter);
        m_results.store(other.m_results);
    }

    return (*this);
}

std::string sink::id() const { return (m_config.id); }

std::string sink::source() const { return (m_config.source); }

api::protocol_counter_flags sink::protocol_counters() const
{
    return (m_config.protocol_counters);
}

api::flow_counter_flags sink::flow_counters() const
{
    return (m_config.flow_counters);
}

api::flow_digest_flags sink::flow_digests() const
{
    return (m_config.flow_digests);
}

size_t sink::worker_count() const { return (m_indexes.size()); }

sink_result* sink::reset(sink_result* results)
{
    results->start();
    auto stopped = m_results.exchange(results, std::memory_order_acq_rel);
    stopped->stop();
    return (stopped);
}

void sink::start(sink_result* results)
{
    results->start();
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
    if (auto* result = m_results.exchange(nullptr, std::memory_order_acq_rel)) {
        result->stop();
    }
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
    constexpr auto signature_counters =
        (statistics::flow_counter_flags::jitter_ipdv
         | statistics::flow_counter_flags::jitter_rfc
         | statistics::flow_counter_flags::latency
         | statistics::flow_counter_flags::sequencing);

    constexpr auto signature_digests =
        (statistics::flow_digest_flags::jitter_ipdv
         | statistics::flow_digest_flags::jitter_rfc
         | statistics::flow_digest_flags::latency
         | statistics::flow_digest_flags::sequence_run_length);

    /* We always need rx_timestamps and RSS hash values */
    auto needed =
        sink_feature_flags::rx_timestamp | sink_feature_flags::rss_hash;

    if (protocol_counters()) {
        needed |= sink_feature_flags::packet_type_decode;
    }

    if ((flow_counters() & signature_counters)
        || (flow_digests() & signature_digests)) {
        needed |= sink_feature_flags::spirent_signature_decode;
    }

    if (flow_counters() & statistics::flow_counter_flags::prbs) {
        /*
         * We need the signature in order to check the bit that
         * indicates that PRBS data is present.
         */
        needed |= (sink_feature_flags::spirent_signature_decode
                   | sink_feature_flags::spirent_prbs_error_detect);
    }

    /* Get features required by the filter */
    if (m_filter) { needed |= bpf::bpf_sink_feature_flags(*m_filter); }

    return (bool(needed & flags));
}

uint16_t sink::push_all(sink_result& results,
                        uint8_t index,
                        const packetio::packet::packet_buffer* const packets[],
                        uint16_t packets_length) const
{
    /* Update protocol statistics */
    auto& protocol = results.protocol(index);
    uint16_t start = 0;
    while (start < packets_length) {
        uint16_t end = start
                       + std::min(static_cast<int>(burst_size_max),
                                  packets_length - start);
        uint16_t count = end - start;

        auto packet_types =
            std::array<packetio::packet::packet_type::flags, burst_size_max>{};
        std::transform(
            packets + start,
            packets + end,
            packet_types.data(),
            [](const auto& packet) {
                return (packetio::packet::packet_type_flags(packet).value);
            });

        protocol.update(packet_types.data(), count);

        start = end;
    }

    /* Update flow statistics */
    auto& flows = results.flow(index);
    auto cache = openperf::utils::flat_memoize<64>(
        [&](uint32_t rss_hash, std::optional<uint32_t> stream_id) {
            return (flows.second.find(rss_hash, stream_id));
        });

    std::for_each(packets, packets + packets_length, [&](const auto& packet) {
        auto hash = packetio::packet::rss_hash(packet);
        auto stream_id = packetio::packet::signature_stream_id(packet);
        auto* counters = cache(hash, stream_id);
        if (!counters) { /* New flow; create stats */
            auto to_delete = flows.second.insert(
                hash,
                stream_id,
                statistics::make_flow_counters(m_config.flow_counters,
                                               m_config.flow_digests));

            counters = cache.retry(hash, stream_id);
            assert(counters);

            counters->set_header(packet);

            flows.first.writer_add_gc_callback([to_delete]() {
                delete to_delete;
                return (sink_result::recycler::gc_callback_result::ok);
            });
        }
        counters->update(packet);
    });

    flows.first.writer_process_gc_callbacks();

    return (packets_length);
}

uint16_t
sink::push_filtered(sink_result& results,
                    uint8_t index,
                    const packetio::packet::packet_buffer* const packets[],
                    uint16_t packets_length) const
{
    const packetio::packet::packet_buffer* const* start = packets;
    std::array<const packetio::packet::packet_buffer*, burst_size_max> filtered;
    auto remain = packets_length;

    while (remain) {
        auto burst_size = std::min(burst_size_max, remain);
        auto length =
            m_filter->filter_burst(start, filtered.data(), burst_size);
        push_all(results, index, filtered.data(), length);
        start += burst_size;
        remain -= burst_size;
    }

    return packets_length;
}

uint16_t sink::push(const packetio::packet::packet_buffer* const packets[],
                    uint16_t packets_length) const
{
    auto results = m_results.load(std::memory_order_consume);
    const auto index = m_indexes[packetio::internal::worker::get_id()];

    return (m_filter ? push_filtered(*results, index, packets, packets_length)
                     : push_all(*results, index, packets, packets_length));
}

} // namespace openperf::packet::analyzer
