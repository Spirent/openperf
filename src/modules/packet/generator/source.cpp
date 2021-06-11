#include "packet/generator/api.hpp"
#include "packet/generator/source.hpp"
#include "packetio/packet_buffer.hpp"
#include "swagger/v1/model/PacketGeneratorConfig.h"
#include "utils/memcpy.hpp"
#include "utils/overloaded_visitor.hpp"

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
    , m_protocols(packet::statistics::make_counters(src.protocol_counters()))
{}

bool source_result::active() const { return (m_active); }

const source& source_result::parent() const { return (m_parent); }

const std::vector<traffic::counter>& source_result::flows() const
{
    return (m_flows);
}

traffic::counter& source_result::operator[](size_t idx)
{
    return (m_flows[idx]);
}

const traffic::counter& source_result::operator[](size_t idx) const
{
    return (m_flows[idx]);
}

const source_result::protocol_counters& source_result::protocols() const
{
    return (m_protocols);
}

source_result::protocol_counters& source_result::protocols()
{
    return (m_protocols);
}

void source_result::start(size_t nb_flows)
{
    m_flows.clear();
    m_flows.resize(nb_flows);
    m_active = true;
}

void source_result::stop() { m_active = false; }

source_helper make_source_helper(packetio::internal::api::client& client,
                                 std::string_view target_id,
                                 [[maybe_unused]] core::event_loop& loop)
{
    auto maybe_port_index = client.get_port_index(target_id);

    if (maybe_port_index.has_value()) { return (port_source{}); }

    if (auto maybe_interface = client.interface(target_id)) {
        return (interface_source{loop, maybe_interface.value()});
    }

    throw std::runtime_error(
        "Unable to find suitable target to create a source_helper for.");
}

source::source(source_config&& config,
               packetio::internal::api::client& client,
               core::event_loop& loop)
    : m_config(config)
    , m_sequence(api::to_sequence(*m_config.api_config))
    , m_load(api::to_load(*m_config.api_config->getLoad(), m_sequence))
    , m_protocols(api::to_protocol_counters_config(
          m_config.api_config->getProtocolCounters()))
    , m_tx_limit(
          api::max_transmit_count(*m_config.api_config->getDuration(), m_load))
    , m_helper(make_source_helper(client, config.target, loop))
{
    if (auto* maybe_intf_helper = std::get_if<interface_source>(&m_helper);
        maybe_intf_helper != nullptr) {
        maybe_intf_helper->populate_source_addresses(m_sequence);
    }
}

source::source(source&& other) noexcept
    : m_config(std::move(other.m_config))
    , m_sequence(std::move(other.m_sequence))
    , m_load(other.m_load)
    , m_protocols(other.m_protocols)
    , m_tx_limit(other.m_tx_limit)
    , m_helper(other.m_helper)
    , m_tx_idx(other.m_tx_idx)
    , m_results(other.m_results.exchange(nullptr))
{}

source& source::operator=(source&& other) noexcept
{
    if (this != &other) {
        m_config = std::move(other.m_config);
        m_sequence = std::move(other.m_sequence);
        m_load = other.m_load;
        m_protocols = other.m_protocols;
        m_tx_limit = other.m_tx_limit;
        m_tx_idx = other.m_tx_idx;
        m_results.store(other.m_results.exchange(nullptr));
        m_helper = other.m_helper;
    }

    return (*this);
}

std::string source::id() const { return (m_config.id); }

std::string source::target() const { return (m_config.target); }

std::string source::target_port() const
{
    std::string to_return;
    std::visit(
        utils::overloaded_visitor(
            [](const std::monostate&) { /* no-op */ },
            [&](const interface_source& intf_src) {
                to_return = intf_src.target_port();
            },
            [&](const port_source& port_src) { to_return = m_config.target; }),
        m_helper);

    return (to_return);
}

api::protocol_counters_config source::protocol_counters() const
{
    return (m_protocols);
}

bool source::active() const
{
    if (m_tx_limit && m_tx_limit.value() == m_tx_idx) {
        if (auto* results = m_results.load(std::memory_order_relaxed)) {
            results->stop();
            m_results.store(nullptr, std::memory_order_relaxed);
        }
        return (false);
    }

    return (m_results.load(std::memory_order_relaxed) != nullptr);
}

bool source::uses_feature(packetio::packet::source_feature_flags flags) const
{
    using source_feature_flags = packetio::packet::source_feature_flags;

    auto needed = openperf::utils::bit_flags<source_feature_flags>{
        source_feature_flags::packet_checksums};

    if (m_sequence.has_signature_config()) {
        needed |= source_feature_flags::spirent_signature_encode;

        if (m_sequence.has_signature_payload_fill()) {
            needed |= source_feature_flags::spirent_payload_fill;
        }
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

    if (results) {
        /* Spin until the generator has stopped transmitting packets */
        auto lock = flag_lock(m_busy);

        results->stop();
    }

    return (results);
}

static size_t to_send_diff(size_t tx_count, size_t tx_limit)
{
    return (tx_count >= tx_limit ? 0 : tx_limit - tx_count);
}

static uint16_t
get_header_length(const packetio::packet::header_lengths& hdr_lens)
{
    return (hdr_lens.layer2 + hdr_lens.layer3 + hdr_lens.layer4
            + hdr_lens.payload);
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

        results->protocols().update(m_packet_scratch.data<3>(), end - start);

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

                /* Copy header into place */
                const auto hdr_len = get_header_length(hdr_lens);
                auto* pkt = packetio::packet::to_data<uint8_t>(buffer);
                utils::memcpy(pkt, hdr_ptr, std::min(hdr_len, pkt_len));

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

                    /* Set optional payload fill */
                    std::visit(
                        utils::overloaded_visitor(
                            [](const std::monostate&) { /* no-op */ },
                            [&](const traffic::signature_const_fill& fill) {
                                packetio::packet::signature_fill_const(
                                    buffer, hdr_len, fill.value);
                            },
                            [&](const traffic::signature_incr_fill& fill) {
                                packetio::packet::signature_fill_incr(
                                    buffer, hdr_len, fill.value);
                            },
                            [&](const traffic::signature_decr_fill& fill) {
                                packetio::packet::signature_fill_decr(
                                    buffer, hdr_len, fill.value);
                            },
                            [&](const traffic::signature_prbs_fill&) {
                                packetio::packet::signature_fill_prbs(buffer,
                                                                      hdr_len);
                            }),
                        sig_config->fill);
                }

                traffic::update(flow_counters, pkt_len, now);

                return (buffer);
            });

        start = end;
    }

    return (to_send);
}

bool source::supports_learning() const
{
    return (
        std::visit(utils::overloaded_visitor(
                       [](const std::monostate&) { return (false); },
                       [](const interface_source& intf_src) { return (true); },
                       [](const port_source& port_src) { return (false); }),
                   m_helper));
}

learning_operation_result source::maybe_retry_learning()
{
    if (auto* maybe_intf_helper = std::get_if<interface_source>(&m_helper);
        maybe_intf_helper != nullptr) {
        if (maybe_intf_helper->retry_learning()) {
            return (learning_operation_result::success);
        }

        return (learning_operation_result::fail);
    }

    return (learning_operation_result::unsupported);
}

learning_operation_result source::maybe_start_learning()
{
    if (auto* maybe_intf_helper = std::get_if<interface_source>(&m_helper);
        maybe_intf_helper != nullptr) {
        if (maybe_intf_helper->start_learning(m_sequence)) {
            return (learning_operation_result::success);
        }

        return (learning_operation_result::fail);
    }

    return (learning_operation_result::unsupported);
}

learning_operation_result source::maybe_stop_learning()
{
    if (auto* maybe_intf_helper = std::get_if<interface_source>(&m_helper);
        maybe_intf_helper != nullptr) {
        maybe_intf_helper->stop_learning();

        return (learning_operation_result::success);
    }

    return (learning_operation_result::unsupported);
}

learning_resolved_state source::maybe_learning_resolved() const
{
    if (auto* maybe_intf_helper = std::get_if<interface_source>(&m_helper);
        maybe_intf_helper != nullptr) {
        return (maybe_intf_helper->learning_state());
    }

    return (learning_resolved_state::unsupported);
}

std::optional<std::reference_wrapper<const learning_state_machine>>
source::maybe_get_learning() const
{
    if (auto* maybe_intf_helper = std::get_if<interface_source>(&m_helper);
        maybe_intf_helper != nullptr) {
        return (std::make_optional(std::cref(maybe_intf_helper->learning())));
    }

    return {};
}

} // namespace openperf::packet::generator
