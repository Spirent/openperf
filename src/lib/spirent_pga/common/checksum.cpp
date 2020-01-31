#include <cassert>
#include <climits>
#include <numeric>
#include <type_traits>

#include <arpa/inet.h>

#include "spirent_pga/functions.h"
#include "checksum.h"
#include "headers.h"

namespace pga::checksum {

inline uint32_t fold64(uint64_t sum)
{
    sum = (sum >> 32) + (sum & 0xffffffff);
    sum += sum >> 32;
    return (static_cast<uint32_t>(sum));
}

inline uint16_t fold32(uint32_t sum)
{
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (static_cast<uint16_t>(sum));
}

uint16_t sum(const uint8_t data[], uint16_t length)
{
    /*
     * Since there are no odd sized headers, data should *always*
     * be aligned to a 16-bit boundary.
     */
    assert((reinterpret_cast<uintptr_t>(data) & 0x1) == 0);

    uint16_t idx = 0;
    uint64_t sum = 0;

    if (reinterpret_cast<uintptr_t>(data) & 0x3) {
        auto data_ptr = reinterpret_cast<const uint16_t*>(data);
        sum = data_ptr[0];
        idx = 2;
    }

    auto& functions = functions::instance();
    auto aligned_length = (length - idx) / 4;

    /* Handing off to our vector functions is never a win for short lengths */
    sum +=
        (aligned_length < 16 /* 16 x 32 bits = 64 bytes */
             ? scalar::checksum_data_aligned(
                 reinterpret_cast<const uint32_t*>(&data[idx]), aligned_length)
             : functions.checksum_data_aligned_impl(
                 reinterpret_cast<const uint32_t*>(&data[idx]),
                 aligned_length));

    /* Handle any trailing unaligned data */
    idx += aligned_length * 4;
    switch ((length - idx) & 0x3) {
    case 1:
        sum += data[idx];
        break;
    case 2:
        sum += data[idx] | (data[idx + 1] << 8);
        break;
    case 3:
        sum += data[idx] | (data[idx + 1] << 8);
        sum += data[idx + 2];
        break;
    }

    /* Finally, reduce the 64 bit sum to a 16 bit value */
    return (fold32(fold64(sum)));
}

void ipv4_headers(const uint8_t* ipv4_headers[],
                  uint16_t count,
                  uint32_t checksums[])
{
    auto& functions = functions::instance();
    functions.checksum_ipv4_headers_impl(ipv4_headers, count, checksums);
}

static void ipv4_pseudoheaders(const uint8_t* ipv4_headers[],
                               uint16_t count,
                               uint32_t checksums[])
{
    auto& functions = functions::instance();
    functions.checksum_ipv4_pseudoheaders_impl(ipv4_headers, count, checksums);
}

void ipv4_tcpudp(const uint8_t* ipv4_headers[],
                 uint16_t count,
                 uint32_t checksums[])
{
    static constexpr int csum_loop_count = 32;
    std::array<uint32_t, csum_loop_count> tmp;

    uint16_t offset = 0;
    while (offset < count) {
        auto loop_count = std::min(csum_loop_count, count - offset);

        /* Calculate the pseudo headers */
        ipv4_pseudoheaders(ipv4_headers + offset, loop_count, tmp.data());

        /*
         * Now add the L4 payload checksum to the pseudoheader checksum and
         * store that in the output vector
         */
        std::transform(
            tmp.data(),
            tmp.data() + loop_count,
            ipv4_headers + offset,
            checksums + offset,
            [](uint32_t& csum, const uint8_t* ptr) -> uint32_t {
                auto ipv4 = reinterpret_cast<const headers::ipv4*>(ptr);
                uint16_t length = ntohs(ipv4->length) - sizeof(headers::ipv4);
                uint32_t total =
                    csum + sum(ptr + sizeof(headers::ipv4), length);
                return (fold32(std::numeric_limits<uint32_t>::max() ^ total));
            });

        offset += loop_count;
    }
}

} // namespace pga::checksum
