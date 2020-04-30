#ifndef _OP_CAPTURE_SINK_HPP_
#define _OP_CAPTURE_SINK_HPP_

#include <optional>
#include <vector>

#include "core/op_core.h"
#include "packet/capture/api.hpp"
#include "packet/capture/capture_buffer.hpp"
#include "packetio/generic_sink.hpp"
#include "utils/recycle.hpp"

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
    bool buffer_wrap;
};

struct sink_result
{
    sink_result(const sink& p);

    capture_buffer_stats get_stats() const;
    bool has_active_transfer() const;

    const sink& parent;
    std::atomic<capture_state> state = capture_state::STOPPED;
    uint64_t start_time;
    uint64_t stop_time;

    std::vector<std::unique_ptr<capture_buffer>> buffers;
    std::unique_ptr<transfer_context> transfer;
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
    void* get_filter() const { return m_filter; }
    void* get_start_trigger() const { return m_start_trigger; }
    void* get_stop_trigger() const { return m_stop_trigger; }

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
    uint16_t check_start_trigger_condition(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) const;
    uint16_t check_stop_trigger_condition(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) const;
    uint16_t check_filter_condition(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length,
        const openperf::packetio::packet::packet_buffer* filtered[]) const;

    std::string m_id;
    std::string m_source;
    capture_mode m_capture_mode;
    bool m_buffer_wrap;
    uint64_t m_buffer_size;
    uint32_t m_max_packet_size;
    uint64_t m_duration;
    void* m_filter = nullptr;        // TODO:
    void* m_start_trigger = nullptr; // TODO:
    void* m_stop_trigger = nullptr;  // TODO:

    std::vector<uint8_t> m_indexes;
    mutable std::atomic<sink_result*> m_results = nullptr;
};

} // namespace openperf::packet::capture

#endif /* _OP_CAPTURE_SINK_HPP_ */
