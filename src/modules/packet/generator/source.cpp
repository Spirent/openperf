#include "packet/generator/api.hpp"
#include "packet/generator/source.hpp"
#include "packetio/packet_buffer.hpp"
#include "utils/memcpy.hpp"

namespace openperf::packet::generator {

using packets_per_hour = packetio::packet::packets_per_hour;

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

bool source::active() const
{
    if (m_tx_limit && m_tx_limit.value() == m_tx_idx) { return (false); }

    return (m_results.load(std::memory_order_relaxed) != nullptr);
}

bool source::uses_feature(packetio::packet::source_feature_flags flags) const
{
    using source_feature_flags = packetio::packet::source_feature_flags;

    auto needed = openperf::utils::bit_flags<source_feature_flags>{
        source_feature_flags::none};

    if (m_sequence.has_signature_config()) {
        needed |= source_feature_flags::spirent_signature_encode;
    }

    return (bool(needed & flags));
}

config_ptr source::config() const { return (m_config.api_config); }

const traffic::sequence& source::sequence() const { return (m_sequence); }

std::optional<size_t> source::tx_limit() const { return (m_tx_limit); }

uint16_t source::max_packet_length() const
{
    return (m_sequence.max_packet_length());
}

uint16_t source::burst_size() const { return (m_load.burst_size); }

packetio::packet::packets_per_hour source::packet_rate() const
{
    return (m_load.rate);
}

void source::start(source_result* results)
{
    results->counters.clear();
    results->counters.resize(m_sequence.flow_count());
    m_tx_idx = 0;
    m_results.store(results, std::memory_order_release);
}

void source::stop() { m_results.store(nullptr, std::memory_order_release); }

static size_t to_send_diff(size_t tx_count, size_t tx_limit)
{
    return (tx_count >= tx_limit ? 0 : tx_limit - tx_count);
}

inline void copy_header(const uint8_t* __restrict src,
                        packetio::packet::header_lengths hdr_len,
                        uint8_t* __restrict dst)
{
    const auto len = hdr_len.layer2 + hdr_len.layer3 + hdr_len.layer4;
    utils::memcpy(dst, src, len);
}

uint16_t source::transform(packetio::packet::packet_buffer* input[],
                           uint16_t input_length,
                           packetio::packet::packet_buffer* output[]) const
{
    auto results = m_results.load(std::memory_order_consume);
    if (!results) { return (0); }

    auto now = traffic::clock_t::now();
    auto to_send = m_tx_limit
                       ? std::min(static_cast<size_t>(input_length),
                                  to_send_diff(m_tx_idx, m_tx_limit.value()))
                       : input_length;

    auto start = 0U;
    while (start < to_send) {
        const auto end = start + std::min(chunk_size, to_send - start);

        m_tx_idx +=
            m_sequence.unpack(m_tx_idx,
                              end - start,
                              m_packet_scratch.data<0>(), /* flow idx */
                              m_packet_scratch.data<1>(), /* header ptr */
                              m_packet_scratch.data<2>(), /* header lengths */
                              m_packet_scratch.data<3>(), /* header flags */
                              m_packet_scratch.data<4>(), /* signature config */
                              m_packet_scratch.data<5>()); /* pkt len */

        std::transform(
            input + start,
            input + end,
            std::begin(m_packet_scratch),
            output + start,
            [&](auto buffer, auto&& pkt_data) {
                auto&& [flow_idx,
                        hdr_ptr,
                        hdr_lens,
                        hdr_flags,
                        sig_config,
                        pkt_len] = pkt_data;

                assert(hdr_flags.value);
                auto pkt = packetio::packet::to_data<uint8_t>(buffer);
                copy_header(hdr_ptr, hdr_lens, pkt);

                /* Set length on both buffer and headers*/
                packetio::packet::length(buffer, pkt_len - 4);
                traffic::update_packet_header_lengths(
                    hdr_ptr, hdr_lens, hdr_flags, pkt_len - 4, pkt);

                /* Set packet type for offloads */
                packetio::packet::tx_offload(buffer, hdr_lens, hdr_flags);

                auto& counters = results->counters[flow_idx];

                if (sig_config) {
                    /*
                     * Conveniently, the per flow packet counter can be used as
                     * the sequence number for our signature frame.
                     */
                    packetio::packet::signature(
                        buffer, sig_config->stream_id, counters.packet, 0);
                }

                traffic::update(counters, pkt_len, now);

                return (buffer);
            });

        start = end;
    }

    return (to_send);
}

} // namespace openperf::packet::generator
