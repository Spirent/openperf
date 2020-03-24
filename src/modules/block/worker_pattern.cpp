#include <unordered_map>
#include "worker_pattern.hpp"
#include "utils/random.hpp"

namespace openperf::block::worker
{

off_t pattern_generator::pattern_blank()
{
    return 0;
}

off_t pattern_generator::pattern_random()
{
    idx = min + utils::random_uniform(max - min);
    return static_cast<off_t>(idx);
}

off_t pattern_generator::pattern_sequential()
{
    if (++idx >= max) {
        idx = min;
    }

    return idx;
}

off_t pattern_generator::pattern_reverse()
{
    if (--idx < min) {
        idx = max - 1;
    }

    return idx;
}

pattern_generator::pattern_generator()
    : min(0)
    , max(0)
    , idx(-1)
{
    generation_method = &pattern_generator::pattern_blank;
}

pattern_generator::pattern_generator(off_t p_min, off_t p_max, model::block_generation_pattern p_pattern)
{
    reset(p_min, p_max, p_pattern);
}

void pattern_generator::reset(off_t p_min, off_t p_max, model::block_generation_pattern p_pattern)
{
    min = p_min;
    max = p_max;
    idx = -1;

    const static std::unordered_map<model::block_generation_pattern, generation_method_t> generation_methods = {
        {model::block_generation_pattern::RANDOM, &pattern_generator::pattern_random},
        {model::block_generation_pattern::REVERSE, &pattern_generator::pattern_reverse},
        {model::block_generation_pattern::SEQUENTIAL, &pattern_generator::pattern_sequential},
    };

    generation_method = generation_methods.at(p_pattern);
}

off_t pattern_generator::generate()
{
    return (this->*generation_method)();
}

}
