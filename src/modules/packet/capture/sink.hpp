#ifndef _OP_CAPTURE_SINK_HPP_
#define _OP_CAPTURE_SINK_HPP_

#include <optional>
#include <vector>

#include "core/op_core.h"
#include "packet/capture/api.hpp"
#include "packetio/generic_sink.hpp"
#include "utils/recycle.hpp"

namespace openperf::packetio::packets {
struct packet_buffer;
}

namespace openperf::packet::capture {

struct sink_config
{
    std::string id = core::to_string(core::uuid::random());
    std::string source;
};

struct sink_result
{
    const sink& parent;

    sink_result(const sink& p);

    std::atomic<int64_t> packets;
    std::atomic<int64_t> bytes;
};

class sink
{
public:
    sink(const sink_config& config, std::vector<unsigned> rx_ids);
    ~sink() = default;

    sink(sink&& other);
    sink& operator=(sink&& other);

    std::string id() const;
    std::string source() const;
    size_t worker_count() const;

    void start(sink_result* results);
    void stop();

    bool active() const;

    bool uses_feature(packetio::packets::sink_feature_flags flags) const;

    uint16_t
    push(const openperf::packetio::packets::packet_buffer* const packets[],
         uint16_t count) const;

private:
    static std::vector<uint8_t> make_indexes(std::vector<unsigned>& ids);

    std::string m_id;
    std::string m_source;
    std::vector<uint8_t> m_indexes;

    mutable std::atomic<sink_result*> m_results = nullptr;
};

} // namespace openperf::packet::capture

#endif /* _OP_CAPTURE_SINK_HPP_ */
