#include <algorithm>
#include <cstdint>

namespace scalar {

template <typename T> inline T count_trailing_zeroes(T value)
{
    return (__builtin_popcount(~value & (value - 1)));
}

void unpack_and_sum_indexicals(const uint32_t indexicals[],
                               uint16_t nb_indexicals,
                               const uint32_t masks[],
                               uint16_t nb_masks,
                               uint64_t* counters[])
{
    std::for_each(
        indexicals, indexicals + nb_indexicals, [&](const auto& value) {
            for (auto idx = 0; idx < nb_masks; idx++) {
                auto shift = count_trailing_zeroes(masks[idx]);
                counters[idx][(value & masks[idx]) >> shift]++;
            }
        });
}

} // namespace scalar
