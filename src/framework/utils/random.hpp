#ifndef _OP_UTILS_RANDOM_HPP_
#define _OP_UTILS_RANDOM_HPP_

#include <random>

namespace openperf::utils {

template <class T = int>
static T random()
{
    T res;
    static std::mt19937_64 generator{std::random_device()()};
    static std::uniform_int_distribution<uint8_t> dist(0, 255);
    uint8_t* data = reinterpret_cast<uint8_t*>(&res);
    for (size_t i = 0; i < sizeof(T); i++) {
        data[i] = dist(generator);
    }
    return res;
}

template <class T>
static T random_uniform(T lower_bound, T upper_bound)
{
    const auto min = std::min(lower_bound, upper_bound);
    const auto max = std::max(lower_bound, upper_bound);
    auto range = max - min;
    auto res = random<T>();
    res = res % range;
    res = (max < 0) ? res + max : res;
    res = (min > 0) ? res + min : res;
    res = (res >= max) ? res - range : res;
    res = (res < min) ? res + range : res;
    return res;
}

template <class T>
static T random_uniform(T max)
{
    return random_uniform(T(0), max);
}

} // namespace openperf::utils

#endif // _OP_UTILS_RANDOM_HPP_
