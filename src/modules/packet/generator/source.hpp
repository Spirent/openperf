#ifndef _OP_PACKET_GENERATOR_SOURCE_HPP_
#define _OP_PACKET_GENERATOR_SOURCE_HPP_

#include "core/op_core.h"
#include "packet/generator/api.hpp"
#include "packet/generator/traffic/counter.hpp"
#include "packet/generator/traffic/sequence.hpp"
#include "packetio/generic_source.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packet::generator {

using config_ptr = std::shared_ptr<swagger::v1::model::PacketGeneratorConfig>;

struct source_config
{
    std::string id = core::to_string(core::uuid::random());
    std::string target;
    /**
     * Unfortunately, transforming a packet generator config into a traffic
     * sequence is a one-way transformation.  Hence, we store the original
     * generator config here so that we can use it for API serialization.
     */
    config_ptr api_config;
};

class source;

class source_result
{
private:
    const source& m_parent;
    std::vector<traffic::counter> m_counters;
    bool m_active = false;

public:
    source_result(const source& p);

    bool active() const;
    const source& parent() const;
    const std::vector<traffic::counter>& counters() const;

    traffic::counter& operator[](size_t idx);
    const traffic::counter& operator[](size_t idx) const;

    void start(size_t nb_counters);
    void stop();
};

struct source_load
{
    uint16_t burst_size;
    api::tx_rate rate;
};

class source
{
public:
    source(source_config&& config);
    ~source() = default;

    source(source&& other) noexcept;
    source& operator=(source&& other) noexcept;

    std::string id() const;
    std::string target() const;
    bool active() const;
    bool uses_feature(packetio::packet::source_feature_flags flags) const;

    config_ptr config() const;
    const traffic::sequence& sequence() const;
    std::optional<size_t> tx_limit() const;

    uint16_t max_packet_length() const;

    uint16_t burst_size() const;
    packetio::packet::packets_per_hour packet_rate() const;

    void start(source_result* results);

    /*
     * Provide a start method that allows callers to specify explicit
     * offsets for signature flows.
     */
    void start(source_result* results,
               const std::map<uint32_t, traffic::stat_t>& sig_stream_offsets);
    source_result* stop();

    uint16_t transform(packetio::packet::packet_buffer* input[],
                       uint16_t input_length,
                       packetio::packet::packet_buffer* output[]) const;

private:
    source_config m_config;
    traffic::sequence m_sequence;
    source_load m_load;
    std::optional<size_t> m_tx_limit;
    std::vector<traffic::stat_t> m_offsets;

    mutable size_t m_tx_idx = 0;
    mutable std::atomic<source_result*> m_results = nullptr;
    mutable std::atomic_flag m_busy = ATOMIC_FLAG_INIT;

    /* Transform packets in chunks of this size */
    static constexpr size_t chunk_size = 64U;

    /*
     * The default constructor of std::array initializes its contents to 0.
     * As a result, we declare our packet data scratch area here so that we
     * initialize the contents only once.  It's silly to do that every time
     * the transmit thread wants packets.
     */
    template <typename T> using chunk_array = std::array<T, chunk_size>;
    using pkt_data_container = openperf::utils::soa_container<
        chunk_array,
        std::tuple<unsigned,       /* flow idx */
                   const uint8_t*, /* header ptr */
                   packetio::packet::header_lengths,
                   packetio::packet::packet_type::flags,
                   std::optional<traffic::signature_config>,
                   uint16_t>>; /* packet length */

    mutable pkt_data_container m_packet_scratch = pkt_data_container{};
};

} // namespace openperf::packet::generator

#endif /* _OP_PACKET_GENERATOR_SOURCE_HPP_ */
