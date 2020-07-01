#include <algorithm>
#include <cassert>
#include <cstdint>

#include "arpa/inet.h"

#include "spirent_pga/common/prbs.h"

namespace scalar {

uint32_t fill_prbs_aligned(uint32_t payload[], uint16_t length, uint32_t seed)
{
    std::generate_n(payload, length, [&]() {
        auto previous = seed;
        seed = pga::prbs::step(seed);
        return (htonl(~previous));
    });

    return (seed);
}

uint64_t verify_prbs_aligned(const uint32_t payload[],
                             uint16_t length,
                             uint32_t expected)
{
    uint32_t bit_errors = 0;
    uint16_t offset = 0;

    /*
     * We start without definitively knowing the PRBS seed value, so we generate
     * future expected values from the payload data directly until
     * we establish a match.  We need two sequential matches to guarantee that
     * we have found the proper PRBS sequence
     */
    unsigned sync_steps = 0;
    while (offset + 1 < length) {
        auto loop_errors =
            __builtin_popcount(ntohl(payload[offset]) ^ ~expected);
        bit_errors += loop_errors;
        sync_steps = (loop_errors ? 0 : sync_steps + 1);
        if (sync_steps > 1) break; /* we've found the PRBS sequence */

        /* Otherwise, use the payload to generate the next seed */
        expected = pga::prbs::step(~ntohl(payload[offset++]));
    }

    /* payload[offset] was checked above */
    std::for_each(
        payload + offset + 1, payload + length, [&](const auto& data) {
            expected = pga::prbs::step(expected);
            bit_errors += __builtin_popcount(ntohl(data) ^ ~expected);
        });

    return (static_cast<uint64_t>(pga::prbs::step(expected)) << 32
            | bit_errors);
}

} // namespace scalar
