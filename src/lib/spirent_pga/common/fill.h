#ifndef _LIB_SPIRENT_PGA_COMMON_FILL_H_
#define _LIB_SPIRENT_PGA_COMMON_FILL_H_

#include <cstdint>

namespace pga::fill {

void     decr(uint8_t payload[], uint16_t length, uint8_t base);
void    fixed(uint8_t payload[], uint16_t length, uint8_t base);
void     incr(uint8_t payload[], uint16_t length, uint8_t base);
uint32_t prbs(uint8_t payload[], uint16_t length, uint32_t seed);

}

#endif /* _LIB_SPIRENT_PGA_COMMON_FILL_H_ */
