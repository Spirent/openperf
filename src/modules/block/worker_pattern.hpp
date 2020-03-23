#ifndef _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_

#include "models/generator.hpp"
#include <unordered_map>

namespace openperf::block::worker
{

class pattern_generator {
private:
    typedef off_t (pattern_generator::*generation_method_t)();
    
    off_t min;
    off_t max;
    off_t idx;
    generation_method_t generation_method;

    off_t pattern_blank();
    off_t pattern_random();
    off_t pattern_sequential();
    off_t pattern_reverse();
public:
    pattern_generator();
    pattern_generator(off_t min, off_t max, model::block_generation_pattern pattern);
    off_t generate();
    void reset(off_t min, off_t max, model::block_generation_pattern pattern);
};

}

#endif // _OP_BLOCK_GENERATOR_WORKER_PATTERN_HPP_