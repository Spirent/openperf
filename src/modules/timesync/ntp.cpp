#include <algorithm>
#include <cinttypes>
#include <cstddef>

#include "timesync/ntp.hpp"

namespace openperf::timesync::ntp {

/*
 * NTPv4 packet header (from RFC 5905)
 *
 *       0                   1                   2                   3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |LI | VN  |Mode |    Stratum     |     Poll      |  Precision   |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                         Root Delay                            |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                         Root Dispersion                       |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                          Reference ID                         |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                                                               |
 *      +                     Reference Timestamp (64)                  +
 *      |                                                               |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                                                               |
 *      +                      Origin Timestamp (64)                    +
 *      |                                                               |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                                                               |
 *      +                      Receive Timestamp (64)                   +
 *      |                                                               |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *      |                                                               |
 *      +                      Transmit Timestamp (64)                  +
 *      |                                                               |
 *      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * NTP uses two similar timestamp formats in the header: a short, 32 bit
 * format and a longer, 64 bit format.
 *
 *       0                   1                   2                   3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |          Seconds              |           Fraction            |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *       0                   1                   2                   3
 *       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                            Seconds                            |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *     |                            Fraction                           |
 *     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

/*
 * NTP timestamps measure seconds since the beginning of the 20th century.
 * UNIX timestamps measure seconds since the beginning of 1970.  Hence,
 * we need to adjust timestamps by a constant fudge factor when converting.
 *
 * Note: There are 17 leap years between 1900 and 1970.
 */
static constexpr unsigned ntp_fudge =
    (((1970U - 1900U) * 365U + 17U) * 24U * 60U * 60U);

/* The only version we support */
static constexpr int ntp_version = 4;

/*
 * Timestamp ser/des routines
 */

using ntp32 = std::array<std::byte, 4>;
using ntp64 = std::array<std::byte, 8>;

static ntp32 to_ntp32(const bintime& from)
{
    auto to = ntp32();
    auto secs = from.bt_sec + ntp_fudge;

    to[0] = static_cast<std::byte>((secs >> 8) & 0xff);
    to[1] = static_cast<std::byte>(secs & 0xff);
    to[2] = static_cast<std::byte>((from.bt_frac >> 56) & 0xff);
    to[3] = static_cast<std::byte>((from.bt_frac >> 48) & 0xff);

    return (to);
}

static ntp64 to_ntp64(const bintime& from)
{
    auto to = ntp64();
    auto secs = from.bt_sec + ntp_fudge;

    to[0] = static_cast<std::byte>((secs >> 24) & 0xff);
    to[1] = static_cast<std::byte>((secs >> 16) & 0xff);
    to[2] = static_cast<std::byte>((secs >> 8) & 0xff);
    to[3] = static_cast<std::byte>(secs & 0xff);
    to[4] = static_cast<std::byte>((from.bt_frac >> 56) & 0xff);
    to[5] = static_cast<std::byte>((from.bt_frac >> 48) & 0xff);
    to[6] = static_cast<std::byte>((from.bt_frac >> 40) & 0xff);
    to[7] = static_cast<std::byte>((from.bt_frac >> 32) & 0xff);

    return (to);
}

static bintime from_ntp32(const std::byte from[4])
{
    using secs_t = decltype(bintime().bt_sec);
    using frac_t = decltype(bintime().bt_frac);

    return (bintime{.bt_sec = (std::to_integer<secs_t>(from[0]) << 8
                               | std::to_integer<secs_t>(from[1]))
                              - ntp_fudge,
                    .bt_frac = std::to_integer<frac_t>(from[2]) << 56UL
                               | std::to_integer<frac_t>(from[3]) << 48UL});
}

static bintime from_ntp64(const std::byte from[8])
{
    using secs_t = decltype(bintime().bt_sec);
    using frac_t = decltype(bintime().bt_frac);

    return (bintime{.bt_sec = (std::to_integer<secs_t>(from[0]) << 24
                               | std::to_integer<secs_t>(from[1]) << 16
                               | std::to_integer<secs_t>(from[2]) << 8
                               | std::to_integer<secs_t>(from[3]))
                              - ntp_fudge,
                    .bt_frac = std::to_integer<frac_t>(from[4]) << 56
                               | std::to_integer<frac_t>(from[5]) << 48
                               | std::to_integer<frac_t>(from[6]) << 40
                               | std::to_integer<frac_t>(from[7]) << 32});
}

std::array<std::byte, packet_size> serialize(const packet& from)
{
    std::array<std::byte, packet_size> to;

    to[0] = static_cast<std::byte>(static_cast<int>(from.leap) << 6
                                   | ntp_version << 3
                                   | static_cast<int>(from.mode));
    to[1] = static_cast<std::byte>(from.stratum);
    to[2] = static_cast<std::byte>(from.poll);
    to[3] = static_cast<std::byte>(from.precision);
    std::copy_n(to_ntp32(from.root.delay).data(), sizeof(ntp32), &to[4]);
    std::copy_n(to_ntp32(from.root.dispersion).data(), sizeof(ntp32), &to[8]);
    std::copy_n(to_ntp64(from.reference).data(), sizeof(ntp64), &to[16]);
    to[20] = static_cast<std::byte>((from.refid >> 24) & 0xff);
    to[21] = static_cast<std::byte>((from.refid >> 16) & 0xff);
    to[22] = static_cast<std::byte>((from.refid >> 8) & 0xff);
    to[23] = static_cast<std::byte>(from.refid & 0xff);
    std::copy_n(to_ntp64(from.origin).data(), sizeof(ntp64), &to[24]);
    std::copy_n(to_ntp64(from.receive).data(), sizeof(ntp64), &to[32]);
    std::copy_n(to_ntp64(from.transmit).data(), sizeof(ntp64), &to[40]);

    return (to);
}

std::optional<packet> deserialize(const std::byte from[], size_t length)
{
    if (length < packet_size) { return (std::nullopt); }

    return (packet{.leap = static_cast<leap_status>(from[0] >> 6),
                   .mode = static_cast<mode>(from[0] & std::byte{7}),
                   .stratum = std::to_integer<uint8_t>(from[1]),
                   .poll = std::to_integer<uint8_t>(from[2]),
                   .precision = std::to_integer<int8_t>(from[3]),
                   .root = {.delay = from_ntp32(&from[4]),
                            .dispersion = from_ntp32(&from[8])},
                   .refid = (std::to_integer<uint32_t>(from[12]) << 24
                             | std::to_integer<uint32_t>(from[13]) << 16
                             | std::to_integer<uint32_t>(from[14]) << 8
                             | std::to_integer<uint32_t>(from[15])),
                   .reference = from_ntp64(&from[16]),
                   .origin = from_ntp64(&from[24]),
                   .receive = from_ntp64(&from[32]),
                   .transmit = from_ntp64(&from[40])});
}

void dump(FILE* f, const packet& packet)
{
    fprintf(f, "Dumping NTPv4 packet (%p)\n", (void*)(std::addressof(packet)));
    fprintf(f, "  stratum: %u\n", packet.stratum);
    fprintf(f, "  poll: %u\n", packet.poll);
    fprintf(f, "  precision: %d\n", packet.precision);
    fprintf(f,
            "  root delay: %04x.%04x\n",
            (uint16_t)(packet.root.delay.bt_sec & 0xffff),
            (uint16_t)(packet.root.delay.bt_frac >> 48));
    fprintf(f,
            "  root dispersion: %04x.%04x\n",
            (uint16_t)(packet.root.dispersion.bt_sec & 0xffff),
            (uint16_t)(packet.root.dispersion.bt_frac >> 48));
    fprintf(f, "  refid: %08x\n", packet.refid);
    fprintf(f,
            "  reference: %016" PRIx64 ".%016" PRIx64 "\n",
            (int64_t)packet.reference.bt_sec,
            packet.reference.bt_frac);
    fprintf(f,
            "  origin: %016" PRIx64 ".%016" PRIx64 "\n",
            (int64_t)packet.origin.bt_sec,
            packet.origin.bt_frac);
    fprintf(f,
            "  receive: %016" PRIx64 ".%016" PRIx64 "\n",
            (int64_t)packet.receive.bt_sec,
            packet.receive.bt_frac);
    fprintf(f,
            "  transmit: %016" PRIx64 ".%016" PRIx64 "\n",
            (int64_t)packet.transmit.bt_sec,
            packet.transmit.bt_frac);
}

} // namespace openperf::timesync::ntp
