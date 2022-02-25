#ifndef _OP_MEMORY_GENERATOR_CONFIG_HPP_
#define _OP_MEMORY_GENERATOR_CONFIG_HPP_

#include "memory/api.hpp"
#include "units/rate.hpp"

namespace openperf::memory::generator {

using ops_per_sec = openperf::units::rate<double, std::ratio<1>>;

struct io_config
{
    ops_per_sec io_rate;
    unsigned io_size;
    unsigned io_threads;
    api::pattern_type io_pattern;
};

struct config
{
    uint64_t buffer_size;
    io_config read;
    io_config write;
};

} // namespace openperf::memory::generator

#endif /* _OP_MEMORY_GENERATOR_CONFIG_HPP_ */
