#include <algorithm>
#include <cassert>
#include <numeric>

#include "arpa/inet.h"

#include "crc.h"
#include "spirent_pga/functions.h"

namespace pga::signature {

struct spirent_signature
{
    uint8_t data[16];
    uint16_t crc;
    uint16_t cheater;
} __attribute__((packed));

static constexpr auto prefetch_offset = 8;

/**
 * Algorithm for transforming an array of data, storing and reducing
 * the result, and pre-fetching the elements as we go.
 */
template <typename InputIt,
          typename OutputIt,
          typename T,
          typename TransformFn,
          typename ReduceFn,
          typename PrefetchFn>
T prefetch_transform_reduce(InputIt cursor,
                            InputIt end,
                            OutputIt d_cursor,
                            T sum,
                            TransformFn&& xform,
                            ReduceFn&& reduce,
                            PrefetchFn&& prefetch,
                            size_t prefetch_offset)
{
    InputIt p_cursor = cursor;
    size_t n = std::distance(cursor, end);

    // Prefetch initial data
    auto stop = cursor + std::min(n, prefetch_offset);
    while (p_cursor != stop) { prefetch(*p_cursor++); }

    // Process items and continue to prefetch
    while (p_cursor != end) {
        prefetch(*p_cursor++);
        *d_cursor = xform(*cursor++);
        sum = reduce(sum, *d_cursor++);
    }

    // All done prefetching so just need to process the rest
    while (cursor != end) {
        *d_cursor = xform(*cursor++);
        sum = reduce(sum, *d_cursor++);
    }

    return (sum);
}

uint16_t
crc_filter(const uint8_t* const payloads[], uint16_t count, int crc_matches[])
{
    /*
     * Unless the user has already done something with the payload, it is
     * unlikely to be in the CPU cache.  Hence, we explicitly prefetch
     * the payload before calculating the CRC.
     */
    return (prefetch_transform_reduce(
        payloads,
        payloads + count,
        crc_matches,
        uint16_t{0},
        [&](const auto* payload) {
            const auto* signature =
                reinterpret_cast<const spirent_signature*>(payload);
            return (ntohs(signature->crc)
                    == calculate_signature_crc16(signature->data));
        },
        std::plus<>(),
        [](const auto* payload) { __builtin_prefetch(payload); },
        prefetch_offset));
}

uint16_t decode(const uint8_t* const payloads[],
                uint16_t count,
                uint32_t stream_ids[],
                uint32_t sequence_numbers[],
                uint64_t timestamps[],
                int flags[])
{
    /* Note: decode_loop_count should be a multiple of all vector widths */
    static constexpr auto decode_loop_count = 32;
    std::array<uint32_t, decode_loop_count> timestamps_hi;
    std::array<uint32_t, decode_loop_count> timestamps_lo;

    auto& functions = functions::instance();

    uint16_t nb_signatures = 0;
    uint16_t begin = 0;

    while (begin < count) {
        auto end = begin + std::min(decode_loop_count, count - begin);
        auto loop_count = end - begin;

        /* Hand the payloads off to our decode function */
        auto loop_signatures =
            functions.decode_signatures_impl(payloads + begin,
                                             loop_count,
                                             stream_ids + begin,
                                             sequence_numbers + begin,
                                             timestamps_hi.data(),
                                             timestamps_lo.data(),
                                             flags + begin);

        /* Then merge hi/lo timestamp data together */
        std::transform(timestamps_hi.data(),
                       timestamps_hi.data() + loop_count,
                       timestamps_lo.data(),
                       timestamps + nb_signatures,
                       [](auto& hi, auto& lo) {
                           /* Only 38 bits of the timestamp are valid */
                           return (static_cast<uint64_t>(hi) << 32 | lo)
                                  & 0x3fffffffff;
                       });

        nb_signatures += loop_signatures;
        begin = end;
    }

    return (nb_signatures);
}

template <typename InputIt, typename OutputIt, typename TransformFn>
std::pair<OutputIt, OutputIt> transform_pair(InputIt first,
                                             InputIt last,
                                             OutputIt cursor1,
                                             OutputIt cursor2,
                                             TransformFn&& xform)
{
    std::for_each(first, last, [&](const auto& item) {
        auto&& pair = xform(item);
        *cursor1++ = pair.first;
        *cursor2++ = pair.second;
    });

    return {cursor1, cursor2};
}

void encode(uint8_t* destinations[],
            const uint32_t stream_ids[],
            const uint32_t sequence_numbers[],
            const uint64_t timestamps[],
            const int flags[],
            uint16_t count)
{
    /* Note: encode_loop_count should be a multiple of all vector widths */
    static constexpr auto encode_loop_count = 32;
    std::array<uint32_t, encode_loop_count> timestamps_hi;
    std::array<uint32_t, encode_loop_count> timestamps_lo;

    auto& functions = functions::instance();

    uint16_t begin = 0;
    while (begin < count) {
        auto end = begin + std::min(encode_loop_count, count - begin);
        auto loop_count = end - begin;

        /* Split the timestamps into hi/lo arrays */
        transform_pair(timestamps + begin,
                       timestamps + end,
                       timestamps_hi.data(),
                       timestamps_lo.data(),
                       [](const auto& ts) -> std::pair<uint32_t, uint32_t> {
                           return {ts >> 32, ts & 0xffffffff};
                       });

        /* Encode some signatures */
        functions.encode_signatures_impl(destinations + begin,
                                         stream_ids + begin,
                                         sequence_numbers + begin,
                                         timestamps_hi.data(),
                                         timestamps_lo.data(),
                                         flags + begin,
                                         loop_count);

        begin = end;
    }

    /*
     * And calculate the CRC for each signature.
     * Note: we just zero out the cheater since we expect hardware to
     * do the payload checksumming for us.
     */
    std::for_each(destinations, destinations + count, [](auto& destination) {
        auto signature = reinterpret_cast<spirent_signature*>(destination);
        signature->crc = htons(calculate_signature_crc16(signature->data));
        signature->cheater = 0;
    });
}

} // namespace pga::signature
