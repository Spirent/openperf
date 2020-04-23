#include <mutex>
#include <thread>
#include <cinttypes>

#include "packetio/internal_client.hpp"
#include "packetio/internal_worker.hpp"
#include "packetio/packet_buffer.hpp"
#include "packet/capture/sink.hpp"

namespace openperf::packet::capture {

std::string to_string(const capture_state& state)
{
    switch (state) {
    case capture_state::STOPPED:
        return "stopped";
    case capture_state::ARMED:
        return "armed";
    case capture_state::STARTED:
        return "started";
    }
}

sink_result::sink_result(const sink& parent_)
    : parent(parent_)
{}

capture_buffer_stats sink_result::get_stats() const
{
    capture_buffer_stats total{0, 0};

    std::for_each(buffers.begin(), buffers.end(), [&](auto& buffer) {
        auto stats = buffer->get_stats();
        total.bytes += stats.bytes;
        total.packets += stats.packets;
    });

    return total;
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
    , m_filter(nullptr)
    , m_trigger(nullptr)
{}

sink::sink(sink&& other)
    : m_id(std::move(other.m_id))
    , m_source(std::move(other.m_source))
    , m_indexes(std::move(other.m_indexes))
    , m_filter(std::move(other.m_filter))
    , m_trigger(std::move(other.m_trigger))
    , m_results(other.m_results.load())
{}

sink& sink::operator=(sink&& other)
{
    if (this != &other) {
        m_id = std::move(other.m_id);
        m_source = std::move(other.m_source);
        m_indexes = std::move(other.m_indexes);
        m_filter = std::move(other.m_filter);
        m_trigger = std::move(other.m_trigger);
        m_results.store(other.m_results);
    }

    return (*this);
}

std::string sink::id() const { return (m_id); }

std::string sink::source() const { return (m_source); }

size_t sink::worker_count() const { return (m_indexes.size()); }

void sink::start(sink_result* results)
{
    // TODO: Add support for trigger condition
    results->state = capture_state::STARTED;

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
    auto results = m_results.load(std::memory_order_consume);
    if (results) {
        if (results->state != capture_state::STOPPED) {
            results->state = capture_state::STOPPED;
        }
        m_results.store(nullptr, std::memory_order_release);
    }
}

bool sink::active() const
{
    return (m_results.load(std::memory_order_relaxed) != nullptr);
}

bool sink::uses_feature(packetio::packets::sink_feature_flags flags) const
{
    using sink_feature_flags = packetio::packets::sink_feature_flags;

    /* We always need rx_timestamps */
    auto needed = openperf::utils::bit_flags<sink_feature_flags>{
        sink_feature_flags::rx_timestamp};

    return (bool(needed & flags));
}

uint16_t sink::check_trigger_condition(
    const packetio::packets::packet_buffer* const packets[],
    uint16_t packets_length) const
{
    // TODO: Add support for trigger
    return 0;
}

uint16_t sink::check_filter_condition(
    const packetio::packets::packet_buffer* const packets[],
    uint16_t packets_length,
    const packetio::packets::packet_buffer* filtered[]) const
{
    // TODO: Add support for filter
    auto end = std::copy_if(packets,
                            packets + packets_length,
                            filtered,
                            [](auto* packet) { return (packet != nullptr); });
    return std::distance(filtered, end);
}

uint16_t sink::push(const packetio::packets::packet_buffer* const packets[],
                    uint16_t packets_length) const
{
    // TODO: add a generic_sink constant for max burst size?
    // burst_size must be set larger than the largest burst which
    // can be passed to a sink
    constexpr int burst_size = 64;
    assert(packets_length < burst_size);

    const auto id = packetio::internal::worker::get_id();

    auto results = m_results.load(std::memory_order_consume);

    if (results) {
        auto state = results->state.load(std::memory_order_consume);
        auto& buffer = results->buffers[m_indexes[id]];
        auto start = packets;
        auto length = packets_length;

        if (state != capture_state::STARTED) {
            if (state == capture_state::ARMED) {
                auto trigger_offset =
                    check_trigger_condition(packets, packets_length);
                if (trigger_offset > packets_length) return packets_length;
                state = capture_state::STARTED;
                results->state = state;
                start += trigger_offset;
                length -= trigger_offset;
            } else {
                return packets_length;
            }
        }

        if (m_filter) {
            std::array<const packetio::packets::packet_buffer*, burst_size>
                filtered;
            length = check_filter_condition(start, length, filtered.data());
            if (length == 0) return packets_length;
            buffer->push(filtered.data(), length);
        } else {
            buffer->push(start, length);
        }
    }

    return (packets_length);
}

} // namespace openperf::packet::capture
