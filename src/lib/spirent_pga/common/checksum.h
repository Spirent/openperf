#ifndef _LIB_SPIRENT_PGA_COMMON_CHECKSUMS_H_
#define _LIB_SPIRENT_PGA_COMMON_CHECKSUMS_H_

#include <cstdint>

namespace pga::checksum {

void ipv4_headers(const uint8_t* const ipv4_headers[],
                  uint16_t count,
                  uint32_t checksums[]);

void ipv4_tcpudp(const uint8_t* const ipv4_headers[],
                 uint16_t count,
                 uint32_t checksums[]);

void ipv6_tcpudp(const uint8_t* const ipv6_headers[],
                 const uint8_t* const payloads[],
                 uint16_t count,
                 uint32_t checksums[]);

} // namespace pga::checksum

#endif /* _LIB_SPIRENT_PGA_COMMON_CHECKSUMS_H_ */
