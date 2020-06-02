#include <mutex>
#include <thread>
#include <cinttypes>

#include "packetio/internal_client.hpp"
#include "packetio/internal_worker.hpp"
#include "packetio/packet_buffer.hpp"
#include "packetio/bpf/bpf.hpp"
#include "packet/capture/sink.hpp"

namespace openperf::packet::capture {

constexpr uint16_t max_burst_size = 64;

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
    , m_filter_str(config.filter)
    , m_start_trigger_str(config.start_trigger)
    , m_stop_trigger_str(config.stop_trigger)
    , m_capture_mode(config.capture_mode)
    , m_buffer_wrap(config.buffer_wrap)
    , m_buffer_size(config.buffer_size)
    , m_max_packet_size(config.max_packet_size)
    , m_duration(config.duration)
    , m_indexes(sink::make_indexes(rx_ids))
{
    if (!m_filter_str.empty())
        m_filter = std::make_unique<openperf::packetio::bpf::bpf>(m_filter_str);
    if (!m_start_trigger_str.empty())
        m_start_trigger =
            std::make_unique<openperf::packetio::bpf::bpf>(m_start_trigger_str);
    if (!m_stop_trigger_str.empty())
        m_stop_trigger =
            std::make_unique<openperf::packetio::bpf::bpf>(m_stop_trigger_str);
}

sink::sink(sink&& other) noexcept
    : m_id(std::move(other.m_id))
    , m_source(std::move(other.m_source))
    , m_filter_str(std::move(other.m_filter_str))
    , m_start_trigger_str(std::move(other.m_start_trigger_str))
    , m_stop_trigger_str(std::move(other.m_stop_trigger_str))
    , m_capture_mode(other.m_capture_mode)
    , m_buffer_wrap(other.m_buffer_wrap)
    , m_buffer_size(other.m_buffer_size)
    , m_max_packet_size(other.m_max_packet_size)
    , m_duration(other.m_duration)
    , m_filter(std::move(other.m_filter))
    , m_start_trigger(std::move(other.m_start_trigger))
    , m_stop_trigger(std::move(other.m_stop_trigger))
    , m_indexes(std::move(other.m_indexes))
    , m_results(other.m_results.load())
{}

sink& sink::operator=(sink&& other) noexcept
{
    if (this != &other) {
        m_id = std::move(other.m_id);
        m_source = std::move(other.m_source);
        m_filter_str = std::move(other.m_filter_str);
        m_start_trigger_str = std::move(other.m_start_trigger_str);
        m_stop_trigger_str = std::move(other.m_stop_trigger_str);
        m_capture_mode = other.m_capture_mode;
        m_buffer_wrap = other.m_buffer_wrap;
        m_buffer_size = other.m_buffer_size;
        m_max_packet_size = other.m_max_packet_size;
        m_duration = other.m_duration;
        m_filter = std::move(other.m_filter);
        m_start_trigger = std::move(other.m_start_trigger);
        m_stop_trigger = std::move(other.m_stop_trigger);
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
    if (!m_start_trigger) {
        set_state(*results, capture_state::STARTED);
    } else {
        set_state(*results, capture_state::ARMED);
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
        set_state(*results, capture_state::STOPPED);
        m_results.store(nullptr, std::memory_order_release);
    }
}

bool sink::active() const
{
    auto result = m_results.load(std::memory_order_relaxed);
    return (result != nullptr && result->state != capture_state::STOPPED);
}

bool sink::uses_feature(packetio::packet::sink_feature_flags flags) const
{
    using sink_feature_flags = packetio::packet::sink_feature_flags;

    /* We always need rx_timestamps */
    auto needed = openperf::utils::bit_flags<sink_feature_flags>{
        sink_feature_flags::rx_timestamp};

    return (bool(needed & flags));
}

void sink::set_state(sink_result& results, capture_state state) const noexcept
{
    auto cur_state = results.state.load(std::memory_order_relaxed);
    if (state == cur_state) return;

    if (results.state.compare_exchange_strong(cur_state, state) == false)
        return;

    if (state == capture_state::STARTED) {
        results.start_time = timesync::chrono::realtime::now();

        if (m_duration) {
            // Set expected stop time
            m_stop_time =
                results.start_time
                + std::chrono::duration<uint64_t, std::milli>{m_duration};
        } else {
            m_stop_time = timesync::chrono::realtime::time_point::max();
        }
    } else if (state == capture_state::STOPPED) {
        results.stop_time = timesync::chrono::realtime::now();
    }

    if (results.state_changed_callback) {
        results.state_changed_callback(results);
    }
}

uint16_t sink::check_start_trigger_condition(
    const packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length) const noexcept
{
    assert(m_start_trigger);
    return m_start_trigger->find_next(packets, packets_length);
}

uint16_t sink::check_stop_trigger_condition(
    const packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length) const noexcept
{
    assert(m_stop_trigger);
    return m_stop_trigger->find_next(packets, packets_length);
}

uint16_t sink::check_filter_condition(
    const packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length,
    const packetio::packet::packet_buffer* filtered[]) const noexcept
{
    std::array<uint64_t, max_burst_size> filter_results;
    uint16_t found = 0;

    assert(m_filter);
    assert(packets_length <= max_burst_size);
    m_filter->exec_burst(packets, filter_results.data(), packets_length);
    for (uint16_t i = 0; i < packets_length; ++i) {
        if (filter_results[i]) { filtered[found++] = packets[i]; }
    }
    return found;
}

uint16_t sink::check_duration_condition(
    const openperf::packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length) const noexcept
{
    auto found =
        std::find_if(packets, packets + packets_length, [&](auto packet) {
            return (openperf::packetio::packet::rx_timestamp(packet)
                    > m_stop_time);
        });
    return std::distance(packets, found);
}

uint16_t sink::push(const packetio::packet::packet_buffer* const packets[],
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
            start += trigger_offset;
            length -= trigger_offset;
            state = capture_state::STARTED;
            set_state(*results, state);
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

    if (m_duration) {
        auto duration_offset = check_duration_condition(packets, length);
        if (duration_offset < length) {
            length = duration_offset + 1;
            stopping = true;
        }
    }

    if (m_filter) {
        std::array<const packetio::packet::packet_buffer*, max_burst_size>
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
        set_state(*results, capture_state::STOPPED);
    }

    return (packets_length);
}

} // namespace openperf::packet::capture
