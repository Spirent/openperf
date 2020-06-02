#ifndef _OP_CAPTURE_SINK_HPP_
#define _OP_CAPTURE_SINK_HPP_

#include <optional>
#include <vector>

#include "core/op_core.h"
#include "packet/capture/api.hpp"
#include "packet/capture/capture_buffer.hpp"
#include "packetio/generic_sink.hpp"
#include "utils/recycle.hpp"

namespace openperf::packetio::bpf {
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
    uint64_t duration;
    uint32_t max_packet_size;
    capture_mode capture_mode;
    std::string filter;
    std::string start_trigger;
    std::string stop_trigger;
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
    std::unique_ptr<transfer_context> transfer;

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

    capture_mode get_capture_mode() const { return m_capture_mode; }
    bool get_buffer_wrap() const { return m_buffer_wrap; }
    uint64_t get_buffer_size() const { return m_buffer_size; }
    uint32_t get_max_packet_size() const { return m_max_packet_size; }
    uint64_t get_duration() const { return m_duration; }
    std::string get_filter_str() const { return m_filter_str; }
    std::string get_start_trigger_str() const { return m_start_trigger_str; }
    std::string get_stop_trigger_str() const { return m_stop_trigger_str; }

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

    uint16_t check_filter_condition(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length,
        const openperf::packetio::packet::packet_buffer* filtered[]) const
        noexcept;

    uint16_t check_duration_condition(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) const noexcept;

    std::string m_id;
    std::string m_source;
    std::string m_filter_str;
    std::string m_start_trigger_str;
    std::string m_stop_trigger_str;

    capture_mode m_capture_mode;
    bool m_buffer_wrap;
    uint64_t m_buffer_size;
    uint32_t m_max_packet_size;
    uint64_t m_duration;

    std::unique_ptr<openperf::packetio::bpf::bpf> m_filter;
    std::unique_ptr<openperf::packetio::bpf::bpf> m_start_trigger;
    std::unique_ptr<openperf::packetio::bpf::bpf> m_stop_trigger;

    std::vector<uint8_t> m_indexes;
    mutable std::atomic<sink_result*> m_results = nullptr;
    mutable timesync::chrono::realtime::time_point m_stop_time;
};

} // namespace openperf::packet::capture

#endif /* _OP_CAPTURE_SINK_HPP_ */
