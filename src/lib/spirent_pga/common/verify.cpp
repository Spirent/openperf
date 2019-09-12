#include "spirent_pga/functions.h"
#include "prbs.h"
#include "verify.h"

namespace pga::verify {

struct verify_results {
    uint32_t seed;
    uint32_t bit_errors;
};

verify_results to_verify_results(uint64_t results)
{
    return (verify_results{static_cast<uint32_t>(results >> 32),
                           static_cast<uint32_t>(results & 0xffffffff)});
}

uint32_t prbs(const uint8_t payload[], uint16_t length)
{
    if (length < 8) return (0);

    auto& functions = functions::instance();

    /*
     * We have no idea what the seed for the incoming PRBS data is, so we presume
     * that the payload is at least partially correct and attempt to recover the
     * seed from it.  This means that we can't actually check the first 32 bits of
     * data for correctness.  However, we might be able to use some of the
     * leading unaligned data to help generate the seed value.
     */
    uint16_t idx = 0;
    uint32_t seed = 0;

    switch (reinterpret_cast<uintptr_t>(payload) & 0x3) {
    case 0: {
        seed = ~(payload[3]   << 24
                 | payload[2] << 16
                 | payload[1] << 8
                 | payload[0]);
        break;
    }
    case 1: {
        seed = ~(payload[2]   << 24
                 | payload[1] << 16
                 | payload[0] << 8
                 | payload[6]);
        auto next = prbs::step(seed);
        seed = seed << 24 | next >> 8;
        idx += 3;
        break;
    }
    case 2: {
        seed = ~(payload[1]   << 24
                 | payload[0] << 16
                 | payload[5] << 8
                 | payload[4]);
        auto next = prbs::step(seed);
        seed = seed << 16 | next >> 16;
        idx += 2;
        break;
    }
    case 3: {
        seed = ~(payload[0]   << 24
                 | payload[4] << 16
                 | payload[3] << 8
                 | payload[2]);
        auto next = prbs::step(seed);
        seed = seed << 8 | next >> 24;
        idx += 1;
        break;
    }
    }

    auto aligned_length = (length - idx) / 4;
    auto results = to_verify_results(functions.verify_prbs_aligned_impl(
                                         reinterpret_cast<const uint32_t*>(&payload[idx]),
                                         aligned_length, seed));

    /*
     * Use the seed returned by the verify function to check any remaining data that
     * was not checked by the aligned function.
     */
    idx += aligned_length * 4;
    switch ((length - idx) & 0x3) {
    case 1:
        results.bit_errors += __builtin_popcount(
            (~results.seed & 0xff) ^ payload[idx]);
        break;
    case 2:
        results.bit_errors += __builtin_popcount(
            (~results.seed & 0xffff) ^ (payload[idx]
                                        | payload[idx + 1] << 8));
        break;
    case 3:
        results.bit_errors += __builtin_popcount(
            (~results.seed & 0xffffff) ^ (payload[idx]
                                          | payload[idx + 1] << 8
                                          | payload[idx + 2] << 16));
        break;
    }

    return (results.bit_errors);
}

}
