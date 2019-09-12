#ifndef _LIB_SPIRENT_PGA_COMMON_VERIFY_H_
#define _LIB_SPIRENT_PGA_COMMON_VERIFY_H_

#include <cstdint>

namespace pga::verify {
uint32_t prbs(const uint8_t payload[], uint16_t length);
}

#endif /* _LIB_SPIRENT_PGA_COMMON_VERIFY_H_ */
