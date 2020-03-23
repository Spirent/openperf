#ifndef _OP_UTILS_RANDOM_HPP_
#define _OP_UTILS_RANDOM_HPP_

#include <random>

namespace openperf::core::utils {

template <class T = int>
static T op_random(T max = std::numeric_limits<T>::max())
{
    static std::mt19937_64 generator{std::random_device()()};
    static std::uniform_int_distribution<uint8_t> dist(0, 255);

    T res;
    uint8_t* data = reinterpret_cast<uint8_t*>(&res);
    for (size_t i = 0; i < sizeof(T); i++) { data[i] = dist(generator); }

    return ((std::is_unsigned<T>::value) ? res : (~res + 1)) % max
           * ((res >= 0) ? 1 : -1);
}

} // namespace openperf::core::utils

#endif // _OP_UTILS_RANDOM_HPP_
