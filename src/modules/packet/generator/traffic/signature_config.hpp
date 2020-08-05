#ifndef _OP_PACKET_GENERATOR_SIGNATURE_CONFIG_HPP_
#define _OP_PACKET_GENERATOR_SIGNATURE_CONFIG_HPP_

#include <cstdint>
#include <variant>

namespace openperf::packet::generator::traffic {

struct signature_const_fill
{
    uint16_t value;
};

struct signature_decr_fill
{
    uint8_t value;
};

struct signature_incr_fill
{
    uint8_t value;
};

struct signature_prbs_fill
{};

using signature_fill = std::variant<std::monostate,
                                    signature_const_fill,
                                    signature_decr_fill,
                                    signature_incr_fill,
                                    signature_prbs_fill>;

struct signature_config
{
    uint32_t stream_id = 0;
    int flags = 0;
    signature_fill fill = std::monostate{};
};

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_SIGNATURE_CONFIG_HPP_ */
