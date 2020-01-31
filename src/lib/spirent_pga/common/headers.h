#ifndef _LIB_SPIRENT_PGA_COMMON_HEADERS_H_
#define _LIB_SPIRENT_PGA_COMMON_HEADERS_H_

#include <cstdint>

namespace pga::headers {

union ipv4
{
    struct
    {
        uint8_t version_ihl;
        uint8_t tos;
        uint16_t length;
        uint16_t identification;
        uint16_t fragment_offset;
        uint8_t ttl;
        uint8_t protocol;
        uint16_t checksum;
        uint32_t src_address;
        uint32_t dst_address;
    };
    uint32_t data[5];
} __attribute__((packed));

union ipv4_pseudo
{
    struct
    {
        uint32_t src_address;
        uint32_t dst_address;
        uint8_t zero;
        uint8_t protocol;
        uint16_t length;
    };
    uint32_t data[3];
} __attribute__((packed));

} // namespace pga::headers

#endif /* _LIB_SPIRENT_PGA_COMMON_HEADERS_H_ */
