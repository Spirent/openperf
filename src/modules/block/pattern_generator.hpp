#ifndef _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_

#include "models/generator.hpp"

namespace openperf::block::worker {

class pattern_generator
{
    using generation_pattern = model::block_generation_pattern;

private:
    using generation_method_t = off_t (pattern_generator::*)();

    off_t m_min = 0;
    off_t m_max = 1;
    off_t m_idx = -1;

    generation_method_t m_generation_method;

public:
    pattern_generator();
    pattern_generator(off_t min, off_t max, generation_pattern pattern);
    off_t generate();
    void reset(off_t min, off_t max, generation_pattern pattern);

private:
    off_t pattern_blank();
    off_t pattern_random();
    off_t pattern_sequential();
    off_t pattern_reverse();
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_