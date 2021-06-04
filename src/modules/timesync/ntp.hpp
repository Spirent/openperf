#ifndef _OP_TIMESYNC_NTP_HPP_
#define _OP_TIMESYNC_NTP_HPP_

#include <array>
#include <cstdint>
#include <netinet/in.h>
#include <optional>

#include "timesync/bintime.hpp"

namespace openperf::timesync::ntp {

enum class leap_status {
    LEAP_NONE = 0,
    LEAP_INSERT = 1,
    LEAP_DELETE = 2,
    LEAP_UNKNOWN = 3
};

enum class mode {
    MODE_MODE_0 = 0,
    MODE_SYMMETRIC_ACTIVE = 1,
    MODE_SYMMETRIC_PASSIVE = 2,
    MODE_CLIENT = 3,
    MODE_SERVER = 4,
    MODE_BROADCAST = 5,
    MODE_CONTROL = 6,
    MODE_MODE7 = 7,
};

enum class kiss_code {
    KISS_DENY = 0x44454e59,
    KISS_RATE = 0x52415445,
    KISS_RSTR = 0x52535452,
};

struct packet
{
    leap_status leap;
    mode mode;
    uint8_t stratum;
    uint8_t poll;
    int8_t precision;
    struct
    {
        bintime delay;
        bintime dispersion;
    } root;
    uint32_t refid;
    bintime reference;
    bintime origin;
    bintime receive;
    bintime transmit;
};

static constexpr in_port_t port = 123;
static constexpr size_t packet_size = 48;

/**
 * Convert a packet representation into wire format.
 */
std::array<std::byte, packet_size> serialize(const packet&);

/**
 * Convert NTP wire data into a packet
 */
std::optional<packet> deserialize(const std::byte[], size_t length);

void dump(FILE*, const packet&);

} // namespace openperf::timesync::ntp

#endif /* _OP_TIMESYNC_NTP_HPP_ */
