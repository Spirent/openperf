#ifndef _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_

#include "models/generator.hpp"

namespace openperf::block::worker {

using generation_pattern = model::block_generation_pattern;

class pattern_generator
{
private:
    typedef off_t (pattern_generator::*generation_method_t)();

    off_t m_min;
    off_t m_max;
    off_t m_idx;
    generation_method_t m_generation_method;

    off_t pattern_blank();
    off_t pattern_random();
    off_t pattern_sequential();
    off_t pattern_reverse();

public:
    pattern_generator();
    pattern_generator(off_t min,
                      off_t max,
                      generation_pattern pattern);
    off_t generate();
    void reset(off_t min, off_t max, generation_pattern pattern);
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_