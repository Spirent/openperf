#include "packet/generator/traffic/protocol/custom.hpp"

namespace openperf::packet::generator::traffic::protocol {

enum custom::field_name custom::get_field_name(std::string_view name) noexcept
{
    return (name == "data" ? custom::field_name::data
                           : custom::field_name::none);
}

header_lengths update_header_lengths(header_lengths& lengths,
                                     const custom& custom) noexcept
{
    switch (custom.layer) {
    case custom::layer_type::ethernet:
        lengths.layer2 += custom.data.size();
        break;
    case custom::layer_type::ip:
        lengths.layer3 += custom.data.size();
        break;
    case custom::layer_type::protocol:
        lengths.layer4 += custom.data.size();
        break;
    case custom::layer_type::payload:
        lengths.payload += custom.data.size();
    default:
        break;
    }

    return (lengths);
}

} // namespace openperf::packet::generator::traffic::protocol
