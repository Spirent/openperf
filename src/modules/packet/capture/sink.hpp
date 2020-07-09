#ifndef _OP_CAPTURE_SINK_HPP_
#define _OP_CAPTURE_SINK_HPP_

#include <optional>
#include <vector>

#include "core/op_core.h"
#include "packet/capture/api.hpp"
#include "packet/capture/capture_buffer.hpp"
#include "packetio/generic_sink.hpp"
#include "utils/recycle.hpp"

namespace openperf::packet::bpf {
class bpf;
}

namespace openperf::packetio::packet {
struct packet_buffer;
}

namespace openperf::packet::capture {

enum class capture_state { STOPPED, ARMED, STARTED };

std::string to_string(const capture_state& state);

enum class capture_mode { BUFFER, LIVE, FILE };

std::string to_string(const capture_mode mode);

capture_mode capture_mode_from_string(const std::string_view str);

struct sink_config
{
    std::string id = core::to_string(core::uuid::random());
    std::string source;
    uint64_t buffer_size;
    uint64_t packet_count;
    uint32_t max_packet_size;
    std::string filter;
    std::string start_trigger;
    std::string stop_trigger;
    std::chrono::duration<uint64_t, std::nano> duration;
    capture_mode capture_mode;
    bool buffer_wrap;
};

struct sink_result
{
    sink_result(const sink& p);

    capture_buffer_stats get_stats() const;
    bool has_active_transfer() const;

    const sink& parent;
    std::atomic<capture_state> state = capture_state::STOPPED;
    timesync::chrono::realtime::time_point start_time;
    timesync::chrono::realtime::time_point stop_time;

    std::vector<std::unique_ptr<capture_buffer>> buffers;
    std::vector<std::unique_ptr<transfer_context>> transfers;

    std::function<void(sink_result&)> state_changed_callback;
    uint32_t timeout_id;
};

class sink
{
public:
    sink(const sink_config& config, std::vector<unsigned> rx_ids);
    ~sink() = default;

    sink(sink&& other) noexcept;
    sink& operator=(sink&& other) noexcept;
    sink(const sink& other) = delete;
    sink& operator=(const sink& other) = delete;

    std::string id() const;
    std::string source() const;

    const sink_config& get_config() const { return m_config; }

    size_t worker_count() const;

    void start(sink_result* results);
    void stop();

    bool active() const;

    sink_result* get_result() const
    {
        return m_results.load(std::memory_order_consume);
    }

    bool uses_feature(packetio::packet::sink_feature_flags flags) const;

    uint16_t
    push(const openperf::packetio::packet::packet_buffer* const packets[],
         uint16_t count) const;

private:
    static std::vector<uint8_t> make_indexes(std::vector<unsigned>& ids);

    void set_state(sink_result& results, capture_state state) const noexcept;

    uint16_t check_start_trigger_condition(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) const noexcept;

    uint16_t check_stop_trigger_condition(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) const noexcept;

    uint16_t check_duration_condition(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) const noexcept;

    uint16_t write_packets(
        capture_buffer& buffer,
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) const noexcept;

    uint16_t write_filtered_packets(
        capture_buffer& buffer,
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) const noexcept;

    sink_config m_config;

    std::unique_ptr<openperf::packet::bpf::bpf> m_filter;
    std::unique_ptr<openperf::packet::bpf::bpf> m_start_trigger;
    std::unique_ptr<openperf::packet::bpf::bpf> m_stop_trigger;

    std::vector<uint8_t> m_indexes;
    mutable std::atomic<sink_result*> m_results = nullptr;
    mutable std::optional<timesync::chrono::realtime::time_point> m_stop_time;
    mutable std::atomic<uint64_t> m_packet_count;
};

} // namespace openperf::packet::capture

#endif /* _OP_CAPTURE_SINK_HPP_ */
