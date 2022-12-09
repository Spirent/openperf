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

void checksum_ipv4_headers(const uint8_t* const ipv4_header_ptrs[],
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

                       const int nb_words = header->data[0] & 0xf;
                       if (nb_words > 5) {
                           for (int word = 5; word < nb_words; word++) {
                               sum += header->data[word];
                           }
                       }

                       uint16_t csum = fold32(fold64(sum));
                       return (csum == 0xffff ? csum : (0xffff ^ csum));
                   });
}

void checksum_ipv4_pseudoheaders(const uint8_t* const ipv4_header_ptrs[],
                                 uint16_t count,
                                 uint32_t checksums[])
{
    std::transform(ipv4_header_ptrs,
                   ipv4_header_ptrs + count,
                   checksums,
                   [](const uint8_t* ptr) {
                       auto ipv4 =
                           reinterpret_cast<const pga::headers::ipv4*>(ptr);
                       const uint8_t ihl = ptr[0] & 0xf;
                       auto pheader = pga::headers::ipv4_pseudo{
                           .src_address = ipv4->src_address,
                           .dst_address = ipv4->dst_address,
                           .zero = 0,
                           .protocol = ipv4->protocol,
                           .length = htons(static_cast<uint16_t>(
                               ntohs(ipv4->length) - (4 * ihl)))};

                       uint64_t sum = pheader.data[0];
                       sum += pheader.data[1];
                       sum += pheader.data[2];

                       return (fold32(fold64(sum)));
                   });
}

uint32_t checksum_data_aligned(const uint32_t data[], uint16_t length)
{
    /*
     * Pro tip: Using 64 bit integers for checksums requires half the
     * instructions compared to using 32 bit integers :)
     */
    const auto* data64 = reinterpret_cast<const uint64_t*>(data);
    auto sum = std::accumulate(data64,
                               data64 + length / 2,
                               uint64_t{0},
                               [](auto&& sum, const auto& value) {
                                   sum += value;
                                   sum += (sum < value); /* handle overflow */
                                   return (sum);
                               });

    /* Don't forget possible trailing data */
    if (length & 0x1) { sum += data[length - 1]; }

    return (fold64(sum));
}

void checksum_ipv6_pseudoheaders(const uint8_t* const ipv6_header_ptrs[],
                                 uint16_t count,
                                 uint32_t checksums[])
{
    std::transform(ipv6_header_ptrs,
                   ipv6_header_ptrs + count,
                   checksums,
                   [](const uint8_t* ptr) {
                       auto ipv6 =
                           reinterpret_cast<const pga::headers::ipv6*>(ptr);
                       auto hdr_sum = fold32(ipv6->payload_length)
                                      + fold32(ntohl(ipv6->protocol));
                       auto addr_sum = checksum_data_aligned(&ipv6->data[2], 8);
                       return (fold32(fold64(hdr_sum + addr_sum)));
                   });
}

} // namespace scalar
