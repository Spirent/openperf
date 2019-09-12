#include <algorithm>
#include <cstdint>

namespace scalar {

uint8_t fill_step_aligned(uint32_t payload[], uint16_t length, uint8_t base, int8_t step)
{
    std::generate_n(payload, length,
                    [&](){
                        auto value = base;
                        base += step * 4;
                        return ((value & 0xff)
                                | ((value + (step * 1)) & 0xff) << 8
                                | ((value + (step * 2)) & 0xff) << 16
                                | ((value + (step * 3)) & 0xff) << 24);
                    });

    return (base);
}

}
