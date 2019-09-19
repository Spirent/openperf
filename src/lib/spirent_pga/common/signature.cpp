#include <algorithm>
#include <cassert>

#include "crc.h"
#include "spirent_pga/functions.h"

namespace pga::signature {

struct spirent_signature {
    uint8_t data[16];
    uint16_t crc;
    uint16_t cheater;
} __attribute__((packed));

template <typename T>
static __attribute__((const)) T distribute(T total, T buckets, T n)
{
    assert(n < buckets);
    auto base = total / buckets;
    return (n < total % buckets ? base + 1 : base);
}

static uint16_t calculate_crc16(const uint8_t data[], uint16_t length)
{
    static constexpr DECLARE_ALIGNED(struct crc_pclmulqdq_ctx spirent_crc16_clmul, 16) = {
        0x45630000,  /**< remainder X^128 / P(X) */
        0xd5f60000,  /**< remainder X^192 / P(X) */
        0xaa510000,  /**< remainder  X^64 / P(X) */
        0x11303471,  /**< quotient   X^64 / P(X) */
        0x10210000,  /**< polynomial */
        0ULL         /**< reserved */
    };

    return (~(crc32_calc_pclmulqdq(data, length, 0xffff,
                                   &spirent_crc16_clmul) >> 16));
}

uint16_t decode(const uint8_t* payloads[], uint16_t count,
                uint32_t stream_ids[],
                uint32_t sequence_numbers[],
                uint64_t timestamps[],
                int flags[])
{
    static constexpr uint16_t decode_loop_count = 32;
    std::array<const uint8_t*, decode_loop_count> signatures;
    std::array<uint32_t, decode_loop_count> timestamp_lo;
    std::array<uint32_t, decode_loop_count> timestamp_hi;

    auto& functions = functions::instance();

    uint16_t nb_loops = (count + (decode_loop_count - 1)) / decode_loop_count;
    uint16_t nb_signatures = 0;
    uint16_t begin = 0;

    for (uint16_t i = 0; i < nb_loops; i++) {
        auto end = begin + distribute(count, nb_loops, i);
        auto copied = 0;

        /* Step 1: Figure out if any of our payloads have valid signature CRCs */
        std::copy_if(payloads + begin, payloads + end,
                     signatures.data(),
                     [&](const auto& payload){
                         auto signature = reinterpret_cast<const spirent_signature*>(payload);
                         auto match = (signature->crc == calculate_crc16(signature->data, 16));
                         copied += match;
                         return (match);
                     });

        /* Step 2: Hand off all matches to our actual analyzer function */
        auto loop_signatures = functions.decode_signatures_impl(signatures.data(), copied,
                                                                stream_ids + nb_signatures,
                                                                sequence_numbers + nb_signatures,
                                                                timestamp_lo.data(),
                                                                timestamp_hi.data(),
                                                                flags + nb_signatures);

        /* Step 3: Merge hi/lo timestamp data together */
        std::transform(timestamp_hi.data(), timestamp_hi.data() + loop_signatures,
                       timestamp_lo.data(),
                       timestamps + nb_signatures,
                       [](auto& hi, auto& lo) {
                           /* Only 38 bits of the timestamp are valid */
                           return (static_cast<uint64_t>(hi) << 32 | lo) & 0x3fffffffff;
                       });

        nb_signatures += loop_signatures;
        begin = end;
    }

    return (nb_signatures);
}

void encode(uint8_t* destinations[],
            const uint32_t stream_ids[], const uint32_t sequence_numbers[],
            uint64_t timestamp, int flags, uint16_t count)
{
    auto& functions = functions::instance();

    /* Write the signature data into place */
    functions.encode_signatures_impl(destinations,
                                     stream_ids, sequence_numbers,
                                     timestamp, flags, count);

    /*
     * And calculate the CRC for each signature.
     * Note: we just zero out the cheater since we expect hardware to
     * do the payload checksumming for us.
     */
    std::for_each(destinations, destinations + count,
                  [](auto& destination){
                      auto signature = reinterpret_cast<spirent_signature*>(destination);
                      signature->crc = calculate_crc16(signature->data, 16);
                      signature->cheater = 0;
                  });
}

}
