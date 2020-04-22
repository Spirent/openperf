#include "packet/generator/api.hpp"
#include "packet/generator/source.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::generator {

using packets_per_hour = packetio::packets::packets_per_hour;

source_result::source_result(const source& src)
    : parent(src)
{}

source::source(source_config&& config)
    : m_config(config)
    , m_sequence(api::to_sequence(*m_config.api_config))
    , m_load(api::to_load(*m_config.api_config->getLoad(), m_sequence))
    , m_tx_limit(
          api::max_transmit_count(*m_config.api_config->getDuration(), m_load))
{}

source::source(source&& other)
    : m_config(std::move(other.m_config))
    , m_sequence(std::move(other.m_sequence))
    , m_load(other.m_load)
    , m_tx_limit(other.m_tx_limit)
    , m_tx_idx(other.m_tx_idx)
    , m_results(other.m_results.exchange(nullptr))
{}

source& source::operator=(source&& other)
{
    if (this != &other) {
        m_config = std::move(other.m_config);
        m_sequence = std::move(other.m_sequence);
        m_load = other.m_load;
        m_tx_limit = other.m_tx_limit;
        m_tx_idx = other.m_tx_idx;
        m_results.store(other.m_results.exchange(nullptr));
    }

    return (*this);
}

std::string source::id() const { return (m_config.id); }

std::string source::target() const { return (m_config.target); }

config_ptr source::config() const { return (m_config.api_config); }

const traffic::sequence& source::sequence() const { return (m_sequence); }

std::optional<size_t> source::tx_limit() const { return (m_tx_limit); }

bool source::active() const
{
    if (m_tx_limit && m_tx_limit.value() == m_tx_idx) { return (false); }

    return (m_results.load(std::memory_order_relaxed) != nullptr);
}

uint16_t source::max_packet_length() const
{
    return (std::visit(
        [](const auto& seq) { return (seq.max_packet_length()); }, m_sequence));
}

uint16_t source::burst_size() const { return (m_load.burst_size); }

packetio::packets::packets_per_hour source::packet_rate() const
{
    return (m_load.rate);
}

void source::start(source_result* results)
{
    results->counters.clear();
    results->counters.resize(std::visit(
        [](const auto& seq) { return (seq.flow_count()); }, m_sequence));
    m_tx_idx = 0;
    m_results.store(results, std::memory_order_release);
}

void source::stop() { m_results.store(nullptr, std::memory_order_release); }

static size_t to_send_diff(size_t tx_count, size_t tx_limit)
{
    return (tx_count >= tx_limit ? 0 : tx_limit - tx_count);
}

uint16_t source::transform(packetio::packets::packet_buffer* input[],
                           uint16_t input_length,
                           packetio::packets::packet_buffer* output[]) const
{
    using namespace openperf::packetio;

    auto results = m_results.load(std::memory_order_consume);
    if (!results) { return (0); }

    auto now = traffic::clock_t::now();
    auto to_send = m_tx_limit
                       ? std::min(static_cast<size_t>(input_length),
                                  to_send_diff(m_tx_idx, m_tx_limit.value()))
                       : input_length;

    std::for_each(input, input + to_send, [&](auto buffer) {
        auto data = packets::to_data<uint8_t>(buffer);

        auto&& [flow_idx, hdr_desc, pkt_len] = std::visit(
            [&](const auto& seq) { return (seq[m_tx_idx++]); }, m_sequence);

        /* Copy the header into place */
        std::copy_n(hdr_desc.ptr, hdr_desc.len, data);

        /* Set lengths on headers */
        traffic::header::update_lengths(hdr_desc.key, data, pkt_len);

        /* Set length on the buffer */
        packets::length(buffer, pkt_len + 4);

        *output++ = buffer;

        traffic::update(results->counters[flow_idx], pkt_len, now);
    });

    return (to_send);
}

} // namespace openperf::packet::generator
