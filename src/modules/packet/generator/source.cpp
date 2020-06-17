#include "packet/generator/api.hpp"
#include "packet/generator/source.hpp"
#include "packetio/packet_buffer.hpp"
#include "swagger/v1/model/PacketGeneratorConfig.h"
#include "utils/memcpy.hpp"

namespace openperf::packet::generator {

using packets_per_hour = packetio::packet::packets_per_hour;

/*
 * Convenient RAII wrappers for using an atomic_flag for notification
 * purposes. Our generator has a worker thread that does actual work
 * and a controller thread that needs to be able to tell if the worker
 * thread is busy.  The worker thread uses the notification struct
 * to atomically set/clear the flag. The controller thread uses the lock
 * struct to verify that the flag is cleared before proceeding.
 */
struct flag_notify
{
    std::atomic_flag& flag_;
    flag_notify(std::atomic_flag& flag)
        : flag_(flag)
    {
        flag_.test_and_set(std::memory_order_acquire);
    }

    ~flag_notify() { flag_.clear(std::memory_order_release); }
};

struct flag_lock
{
    std::atomic_flag& flag_;
    flag_lock(std::atomic_flag& flag)
        : flag_(flag)
    {
        while (flag_.test_and_set(std::memory_order_acquire))
            ;
    }

    ~flag_lock() { flag_.clear(std::memory_order_release); }
};

source_result::source_result(const source& src)
    : m_parent(src)
{}

bool source_result::active() const { return (m_active); }

const source& source_result::parent() const { return (m_parent); }

const std::vector<traffic::counter>& source_result::counters() const
{
    return (m_counters);
}

traffic::counter& source_result::operator[](size_t idx)
{
    return (m_counters[idx]);
}

const traffic::counter& source_result::operator[](size_t idx) const
{
    return (m_counters[idx]);
}

void source_result::start(size_t nb_counters)
{
    m_counters.clear();
    m_counters.resize(nb_counters);
    m_active = true;
}

void source_result::stop() { m_active = false; }

source::source(source_config&& config)
    : m_config(config)
    , m_sequence(api::to_sequence(*m_config.api_config))
    , m_load(api::to_load(*m_config.api_config->getLoad(), m_sequence))
    , m_tx_limit(
          api::max_transmit_count(*m_config.api_config->getDuration(), m_load))
{}

source::source(source&& other) noexcept
    : m_config(std::move(other.m_config))
    , m_sequence(std::move(other.m_sequence))
    , m_load(other.m_load)
    , m_tx_limit(other.m_tx_limit)
    , m_tx_idx(other.m_tx_idx)
    , m_results(other.m_results.exchange(nullptr))
{}

source& source::operator=(source&& other) noexcept
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
    if (m_tx_limit && m_tx_limit.value() == m_tx_idx) {
        if (auto* results = m_results.load(std::memory_order_relaxed)) {
            results->stop();
        }
        return (false);
    }

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
    m_tx_idx = 0;
    m_offsets.resize(m_sequence.flow_count()); /* no offsets */
    results->start(m_sequence.flow_count());
    m_results.store(results, std::memory_order_release);
}

void source::start(
    source_result* results,
    const std::map<uint32_t, traffic::stat_t>& sig_stream_offsets)
{
    using sig_config_type = std::optional<traffic::signature_config>;

    m_tx_idx = 0;
    m_offsets.resize(m_sequence.flow_count());

    /* Merge incoming offset data */
    std::for_each(
        std::begin(m_sequence),
        std::end(m_sequence),
        [&](const auto& flow_tuple) {
            const auto& sig_config = std::get<sig_config_type>(flow_tuple);
            if (sig_config && sig_stream_offsets.count(sig_config->stream_id)) {
                m_offsets[std::get<0>(flow_tuple)] =
                    sig_stream_offsets.at(sig_config->stream_id);
            }
        });

    start(results);
    results->start(m_sequence.flow_count());
    m_results.store(results, std::memory_order_release);
}

source_result* source::stop()
{
    auto* results = m_results.exchange(nullptr, std::memory_order_acq_rel);

    /* Spin until the generator has finished transmitting any last packets */
    auto lock = flag_lock(m_busy);

    results->stop();
    return (results);
}

static size_t to_send_diff(size_t tx_count, size_t tx_limit)
{
    return (tx_count >= tx_limit ? 0 : tx_limit - tx_count);
}

inline void copy_header(const uint8_t* __restrict src,
                        packetio::packet::header_lengths hdr_len,
                        uint8_t* __restrict dst,
                        int pkt_len)
{
    const auto len =
        hdr_len.layer2 + hdr_len.layer3 + hdr_len.layer4 + hdr_len.payload;
    utils::memcpy(dst, src, std::min(len, pkt_len));
}

uint16_t source::transform(packetio::packet::packet_buffer* input[],
                           uint16_t input_length,
                           packetio::packet::packet_buffer* output[]) const
{
    auto notify = flag_notify(m_busy);

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

                auto pkt = packetio::packet::to_data<uint8_t>(buffer);
                copy_header(hdr_ptr, hdr_lens, pkt, pkt_len);

                /* Set length on both buffer and headers*/
                packetio::packet::length(buffer, pkt_len - 4);
                traffic::update_packet_header_lengths(
                    hdr_ptr, hdr_lens, hdr_flags, pkt_len - 4, pkt);

                /* Set packet type for offloads */
                packetio::packet::tx_offload(buffer, hdr_lens, hdr_flags);

                auto&& flow_counters = (*results)[flow_idx];

                if (sig_config) {
                    /*
                     * Conveniently, the per flow packet counter can be used as
                     * the sequence number for our signature frame.
                     */
                    packetio::packet::signature(buffer,
                                                sig_config->stream_id,
                                                m_offsets[flow_idx]
                                                    + flow_counters.packet,
                                                sig_config->flags);
                }

                traffic::update(flow_counters, pkt_len, now);

                return (buffer);
            });

        start = end;
    }

    return (to_send);
}

} // namespace openperf::packet::generator
