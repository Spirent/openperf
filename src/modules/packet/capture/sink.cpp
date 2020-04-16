#include <mutex>
#include <thread>

#include "packetio/internal_client.hpp"
#include "packetio/internal_worker.hpp"
#include "packetio/packet_buffer.hpp"
#include "packet/capture/sink.hpp"
// #include "utils/flat_memoize.hpp"

namespace openperf::packet::capture {

sink_result::sink_result(const sink& parent_)
    : parent(parent_)
{}

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
{}

sink::sink(sink&& other)
    : m_id(std::move(other.m_id))
    , m_source(std::move(other.m_source))
    , m_indexes(std::move(other.m_indexes))
{}

sink& sink::operator=(sink&& other)
{
    if (this != &other) {
        m_id = std::move(other.m_id);
        m_source = std::move(other.m_source);
        m_indexes = std::move(other.m_indexes);
    }

    return (*this);
}

std::string sink::id() const { return (m_id); }

std::string sink::source() const { return (m_source); }

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
}

bool sink::uses_feature(packetio::packets::sink_feature_flags flags) const
{
    using sink_feature_flags = packetio::packets::sink_feature_flags;

    /* We always need rx_timestamps */
    auto needed = openperf::utils::bit_flags<sink_feature_flags>{
        sink_feature_flags::rx_timestamp};

    return (bool(needed & flags));
}

uint16_t
sink::push(const openperf::packetio::packets::packet_buffer* const packets[],
           uint16_t packets_length) const
{
    using namespace openperf::packetio;
    // const auto id = internal::worker::get_id();
    // auto results = m_results.load(std::memory_order_consume);

    std::for_each(packets, packets + packets_length, [&](const auto& packet) {
        // TODO: Capture packets
    });

    return (packets_length);
}

} // namespace openperf::packet::capture
