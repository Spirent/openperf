#include <algorithm>
#include <cstdint>
#include <numeric>

#include <arpa/inet.h>

#include "spirent_pga/common/headers.h"

namespace scalar {

inline uint32_t fold64(uint64_t sum)
{
    sum = (sum >> 32) + (sum & 0xffffffff);
    sum += sum >> 32;
    return (static_cast<uint32_t>(sum));
}

inline uint16_t fold32(uint32_t sum)
{
    sum = (sum >> 16) + (sum & 0xffff);
    sum += sum >> 16;
    return (static_cast<uint16_t>(sum));
}

void checksum_ipv4_headers(const uint8_t* ipv4_header_ptrs[],
                           uint16_t count,
                           uint32_t checksums[])
{
    std::transform(ipv4_header_ptrs,
                   ipv4_header_ptrs + count,
                   checksums,
                   [](const uint8_t* ptr) {
                       auto header =
                           reinterpret_cast<const pga::headers::ipv4*>(ptr);

                       uint64_t sum = header->data[0];
                       sum += header->data[1];
                       sum += header->data[2];
                       sum += header->data[3];
                       sum += header->data[4];

                       uint16_t csum = fold32(fold64(sum));
                       return (csum == 0xffff ? csum : (0xffff ^ csum));
                   });
}

void checksum_ipv4_pseudoheaders(const uint8_t* ipv4_header_ptrs[],
                                 uint16_t count,
                                 uint32_t checksums[])
{
    std::transform(
        ipv4_header_ptrs,
        ipv4_header_ptrs + count,
        checksums,
        [](const uint8_t* ptr) {
            auto ipv4 = reinterpret_cast<const pga::headers::ipv4*>(ptr);

            auto pheader = pga::headers::ipv4_pseudo{
                .src_address = ipv4->src_address,
                .dst_address = ipv4->dst_address,
                .zero = 0,
                .protocol = ipv4->protocol,
                .length = htons(static_cast<uint16_t>(
                    ntohs(ipv4->length) - sizeof(pga::headers::ipv4)))};

            uint64_t sum = pheader.data[0];
            sum += pheader.data[1];
            sum += pheader.data[2];

            return (fold32(fold64(sum)));
        });
}

uint32_t checksum_data_aligned(const uint32_t data[], uint16_t length)
{
    uint64_t sum = std::accumulate(
        data,
        data + length,
        uint64_t{0},
        [](const auto& left, const auto& right) { return (left + right); });
    return (fold64(sum));
}

} // namespace scalar
