#ifndef _OP_UTILS_RANDOM_HPP_
#define _OP_UTILS_RANDOM_HPP_

#include <random>

namespace openperf::utils {

template <class T> static T random_uniform(T lower_bound, T upper_bound)
{
    static std::mt19937_64 generator{std::random_device()()};
    std::uniform_int_distribution<T> dist(lower_bound, upper_bound - 1);
    return dist(generator);
}

template <class T> static T random_uniform(T max)
{
    return random_uniform(T(0), max);
}

} // namespace openperf::utils

#endif // _OP_UTILS_RANDOM_HPP_
