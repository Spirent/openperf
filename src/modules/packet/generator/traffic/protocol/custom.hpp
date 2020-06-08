#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_CUSTOM_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_CUSTOM_HPP_

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

#include "packetio/packet_buffer.hpp"

namespace openperf::packet::generator::traffic::protocol {

struct custom
{
    static constexpr size_t protocol_field_count = 1;
    static constexpr std::string_view protocol_name = "custom";

    enum class field_name { none, data };

    enum layer_type {
        none,
        ethernet,
        ip,
        protocol,
        payload,
    };

    std::vector<uint8_t> data;

    /* Metadata */
    std::optional<uint16_t> protocol_id = std::nullopt;
    layer_type layer = layer_type::none;

    static enum custom::field_name
    get_field_name(std::string_view name) noexcept;
};

using header_lengths = packetio::packet::header_lengths;
header_lengths update_header_lengths(header_lengths& lengths,
                                     const custom&) noexcept;

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_CUSTOM_HPP_ */
