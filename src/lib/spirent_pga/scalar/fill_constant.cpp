#include <algorithm>
#include <cstdint>

namespace scalar {

void fill_constant_aligned(uint32_t payload[], uint16_t length, uint32_t value)
{
    std::fill_n(payload, length, value);
}

} // namespace scalar
