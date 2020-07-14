#include <mutex>
#include <thread>
#include <cinttypes>

#include "packetio/internal_client.hpp"
#include "packetio/internal_worker.hpp"
#include "packetio/packet_buffer.hpp"
#include "packet/bpf/bpf.hpp"
#include "packet/bpf/bpf_sink.hpp"
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
    return std::any_of(transfers.begin(), transfers.end(), [](auto& transfer) {
        return (!transfer->is_done());
    });
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
    : m_config(config)
    , m_indexes(sink::make_indexes(rx_ids))
{
    if (!m_config.filter.empty()) {
        m_filter =
            std::make_unique<openperf::packet::bpf::bpf>(m_config.filter);

        auto bpf_filter_flags = m_filter->get_filter_flags();
        auto bpf_features = bpf::bpf_sink_feature_flags(bpf_filter_flags);
        OP_LOG(OP_LOG_DEBUG,
               "Capture BPF filter '%s' flags %#x sink_features %#x",
               m_config.filter.c_str(),
               bpf_filter_flags,
               bpf_features.value);
    }
    if (!m_config.start_trigger.empty())
        m_start_trigger = std::make_unique<openperf::packet::bpf::bpf>(
            m_config.start_trigger);
    if (!m_config.stop_trigger.empty())
        m_stop_trigger =
            std::make_unique<openperf::packet::bpf::bpf>(m_config.stop_trigger);
}

sink::sink(sink&& other) noexcept
    : m_config(std::move(other.m_config))
    , m_filter(std::move(other.m_filter))
    , m_start_trigger(std::move(other.m_start_trigger))
    , m_stop_trigger(std::move(other.m_stop_trigger))
    , m_indexes(std::move(other.m_indexes))
    , m_results(other.m_results.load())
    , m_stop_time(other.m_stop_time)
{}

sink& sink::operator=(sink&& other) noexcept
{
    if (this != &other) {
        m_config = std::move(other.m_config);
        m_filter = std::move(other.m_filter);
        m_start_trigger = std::move(other.m_start_trigger);
        m_stop_trigger = std::move(other.m_stop_trigger);
        m_indexes = std::move(other.m_indexes);
        m_results.store(other.m_results);
        m_stop_time = other.m_stop_time;
    }

    return (*this);
}

std::string sink::id() const { return (m_config.id); }

std::string sink::source() const { return (m_config.source); }

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

    /* Get features required for all filters */
    if (m_filter)
        needed |= openperf::packet::bpf::bpf_sink_feature_flags(*m_filter);
    if (m_start_trigger)
        needed |=
            openperf::packet::bpf::bpf_sink_feature_flags(*m_start_trigger);
    if (m_stop_trigger)
        needed |=
            openperf::packet::bpf::bpf_sink_feature_flags(*m_stop_trigger);

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

        if (m_config.duration.count()) {
            // Set expected stop time
            m_stop_time = results.start_time + m_config.duration;
        } else {
            m_stop_time = {};
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

uint16_t sink::check_duration_condition(
    const openperf::packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length) const noexcept
{
    auto found = std::find_if(
        packets,
        packets + packets_length,
        [stop_time = m_stop_time.value()](auto packet) {
            return (openperf::packetio::packet::rx_timestamp(packet)
                    > stop_time);
        });
    return std::distance(packets, found);
}

/**
 * Increment the counter value atomically up to the specified limit.
 * @param[in] counter The counter to increment
 * @param[in] count The amount to increment the counter
 * @param[in] lim The limit to increment the counter up to
 * @return The amount the counter was incremented.
 */
static uint16_t
increment_counter(std::atomic<uint64_t>& counter, uint16_t count, uint64_t lim)
{
    uint64_t value = counter.load(std::memory_order_consume);
    while (value != lim) {
        uint64_t new_value = std::min(value + count, lim);
        if (counter.compare_exchange_strong(value, new_value)) {
            return (new_value - value);
        }
    }
    return 0;
}

uint16_t sink::write_packets(
    capture_buffer& buffer,
    const openperf::packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length) const noexcept
{
    if (!m_config.packet_count) {
        return buffer.write_packets(packets, packets_length);
    } else {
        // The packet counter needs to be incremented atomically
        // before adding packets to the buffer to ensure that the
        // packet count limit across all threads is not exceeded.
        //
        // Atomic operations are slow, so only do it when the packet count
        // limit is used.
        auto packets_added = increment_counter(
            m_packet_count, packets_length, m_config.packet_count);
        return buffer.write_packets(packets, packets_added);
    }
}

uint16_t sink::write_filtered_packets(
    capture_buffer& buffer,
    const openperf::packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length) const noexcept
{
    std::array<const packetio::packet::packet_buffer*, max_burst_size> filtered;
    auto start = packets;
    auto remain = packets_length;
    while (remain) {
        auto burst_size = std::min(max_burst_size, remain);
        auto length =
            m_filter->filter_burst(start, filtered.data(), burst_size);
        if (write_packets(buffer, filtered.data(), length) != length) {
            // Return number of packets in full bursts which were processed.
            // Currently callers only need to know that processing ended
            // early so this is good enough.
            return packets_length - remain;
        }
        start += burst_size;
        remain -= burst_size;
    }
    return packets_length;
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

    if (m_stop_time) {
        auto duration_offset = check_duration_condition(packets, length);
        if (duration_offset < length) {
            length = duration_offset + 1;
            stopping = true;
        }
    }

    if (m_filter) {
        if (write_filtered_packets(*buffer, start, length) != length) {
            stopping = true;
        }
    } else {
        if (write_packets(*buffer, start, length) != length) {
            stopping = true;
        }
    }

    if (stopping || buffer->is_full()) {
        set_state(*results, capture_state::STOPPED);
    }

    return (packets_length);
}

} // namespace openperf::packet::capture
