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

std::string to_string(const capture_mode mode)
{
    switch (mode) {
    case capture_mode::BUFFER:
        return "buffer";
    case capture_mode::LIVE:
        return "live";
    case capture_mode::FILE:
        return "file";
    }
}

capture_mode capture_mode_from_string(const std::string_view str)
{
    if (str == "buffer") { return capture_mode::BUFFER; }
    if (str == "live") { return capture_mode::LIVE; }
    if (str == "file") { return capture_mode::FILE; }
    return capture_mode::BUFFER;
}

sink_result::sink_result(const sink& parent_)
    : parent(parent_)
    , state(capture_state::STOPPED)
    , start_time(0)
    , stop_time(0)
    , timeout_id(0)
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

bool sink_result::has_active_transfer() const
{
    return (transfer.get() && !transfer->is_done());
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
    , m_capture_mode(config.capture_mode)
    , m_buffer_wrap(config.buffer_wrap)
    , m_buffer_size(config.buffer_size)
    , m_max_packet_size(config.max_packet_size)
    , m_duration(config.duration)
    , m_indexes(sink::make_indexes(rx_ids))
{}

sink::sink(sink&& other) noexcept
    : m_id(std::move(other.m_id))
    , m_source(std::move(other.m_source))
    , m_capture_mode(other.m_capture_mode)
    , m_buffer_wrap(other.m_buffer_wrap)
    , m_buffer_size(other.m_buffer_size)
    , m_max_packet_size(other.m_max_packet_size)
    , m_duration(other.m_duration)
    , m_filter(other.m_filter)
    , m_start_trigger(other.m_start_trigger)
    , m_stop_trigger(other.m_stop_trigger)
    , m_indexes(std::move(other.m_indexes))
    , m_results(other.m_results.load())
{}

sink& sink::operator=(sink&& other) noexcept
{
    if (this != &other) {
        m_id = std::move(other.m_id);
        m_source = std::move(other.m_source);
        m_capture_mode = other.m_capture_mode;
        m_buffer_wrap = other.m_buffer_wrap;
        m_buffer_size = other.m_buffer_size;
        m_max_packet_size = other.m_max_packet_size;
        m_duration = other.m_duration;
        m_filter = other.m_filter;
        m_start_trigger = other.m_start_trigger;
        m_stop_trigger = other.m_stop_trigger;
        m_indexes = std::move(other.m_indexes);
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
    results->start_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    // Set expected stop time
    // TODO:
    // Ideally this will be in same time units/base as packet timestamp
    // so sink can discard packets after stop_time and start trigger
    // can determine stop_time from duration and packet timestamp.
    if (m_duration) {
        results->stop_time = results->start_time + m_duration;
    } else {
        results->stop_time = UINT64_MAX;
    }

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
        // TODO: This will not be thread safe when start/stop triggers are added
        if (results->state != capture_state::STOPPED) {
            results->state = capture_state::STOPPED;
            results->stop_time =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
        }
        m_results.store(nullptr, std::memory_order_release);
    }
}

bool sink::active() const
{
    auto result = m_results.load(std::memory_order_relaxed);
    return (result != nullptr && result->state != capture_state::STOPPED);
}

bool sink::uses_feature(packetio::packets::sink_feature_flags flags) const
{
    using sink_feature_flags = packetio::packets::sink_feature_flags;

    /* We always need rx_timestamps */
    auto needed = openperf::utils::bit_flags<sink_feature_flags>{
        sink_feature_flags::rx_timestamp};

    return (bool(needed & flags));
}

uint16_t sink::check_start_trigger_condition(
    const packetio::packets::packet_buffer* const packets[],
    uint16_t packets_length) const
{
    // TODO: Add support for trigger
    return 0;
}

uint16_t sink::check_stop_trigger_condition(
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
    const auto id = packetio::internal::worker::get_id();

    auto results = m_results.load(std::memory_order_consume);

    if (!results) { return (0); }

    auto state = results->state.load(std::memory_order_consume);
    auto& buffer = results->buffers[m_indexes[id]];
    auto start = packets;
    auto length = packets_length;
    bool stopping = false;

    if (state != capture_state::STARTED) {
        if (state == capture_state::ARMED) {
            auto trigger_offset =
                check_start_trigger_condition(packets, packets_length);
            if (trigger_offset >= packets_length) {
                // Start not triggered yet
                return packets_length;
            }
            state = capture_state::STARTED;
            results->state = state;
            start += trigger_offset;
            length -= trigger_offset;
        } else {
            return packets_length;
        }
    }

    if (m_stop_trigger) {
        auto trigger_offset = check_stop_trigger_condition(packets, length);
        if (trigger_offset < length) {
            length = trigger_offset + 1;
            stopping = true;
        }
    }

    if (m_filter) {
        constexpr uint16_t max_burst_size = 64;
        std::array<const packetio::packets::packet_buffer*, max_burst_size>
            filtered;
        auto remain = length;
        while (remain) {
            auto burst_size = std::min(max_burst_size, remain);
            length = check_filter_condition(start, burst_size, filtered.data());
            if (length) buffer->write_packets(filtered.data(), length);
            start += burst_size;
            remain -= burst_size;
        }
    } else {
        buffer->write_packets(start, length);
    }

    if (stopping || buffer->is_full()) {
        results->state = capture_state::STOPPED;
    }

    return (packets_length);
}

} // namespace openperf::packet::capture
