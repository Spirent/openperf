#include <algorithm>
#include <cassert>
#include <cstdint>

#include "spirent_pga/common/prbs.h"

namespace scalar {

uint32_t fill_prbs_aligned(uint32_t payload[], uint16_t length, uint32_t seed)
{
    std::generate_n(payload, length,
                    [&](){
                        auto previous = seed;
                        seed = pga::prbs::step(seed);
                        return (~previous);
                    });

    return (seed);
}

uint64_t verify_prbs_aligned(const uint32_t payload[], uint16_t length,
                             uint32_t expected)
{
    uint32_t bit_errors = 0;
    uint16_t offset = 0;

    /*
     * We start without definitively knowing the PRBS seed value, so we generate
     * future expected values from the payload data directly until
     * we establish a match.
     */
    while (offset + 1 < length) {
        auto loop_errors = __builtin_popcount(payload[offset++] ^ ~expected);
        bit_errors += loop_errors;
        if (!loop_errors) break;  /* we've found the PRBS sequence */

        /* Otherwise, use the payload to generate the next seed */
        expected = pga::prbs::step(~payload[offset]);
    }

    std::for_each(payload + offset, payload + length,
                  [&](auto data) {
                      expected = pga::prbs::step(expected);
                      bit_errors += __builtin_popcount(data ^ ~expected);
                  });

    return (static_cast<uint64_t>(pga::prbs::step(expected)) << 32 | bit_errors);
}

}
