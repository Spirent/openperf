#ifndef _OP_PACKET_GENERATOR_SOURCE_HPP_
#define _OP_PACKET_GENERATOR_SOURCE_HPP_

#include "core/op_core.h"
#include "packet/generator/traffic/counter.hpp"
#include "packet/generator/traffic/sequence.hpp"
#include "packetio/generic_source.hpp"
#include "swagger/v1/model/PacketGeneratorConfig.h"
#include "units/rate.hpp"

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

struct source_result
{
    const source& parent;
    std::vector<traffic::counter> counters;

    source_result(const source& p);
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

    source(source&& other);
    source& operator=(source&& other);

    std::string id() const;
    std::string target() const;
    bool active() const;

    config_ptr config() const;

    const traffic::sequence& sequence() const;

    std::optional<size_t> tx_limit() const;

    uint16_t max_packet_length() const;

    uint16_t burst_size() const;
    packetio::packets::packets_per_hour packet_rate() const;

    void start(source_result* results);
    void stop();

    uint16_t transform(packetio::packets::packet_buffer* input[],
                       uint16_t input_length,
                       packetio::packets::packet_buffer* output[]) const;

private:
    source_config m_config;
    traffic::sequence m_sequence;
    source_load m_load;
    std::optional<size_t> m_tx_limit;

    mutable size_t m_tx_idx = 0;
    mutable std::atomic<source_result*> m_results = nullptr;
};

} // namespace openperf::packet::generator

#endif /* _OP_PACKET_GENERATOR_SOURCE_HPP_ */
