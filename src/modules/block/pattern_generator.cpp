#include <unordered_map>
#include "pattern_generator.hpp"
#include "utils/random.hpp"

namespace openperf::block::worker {

off_t pattern_generator::pattern_blank() { return 0; }

off_t pattern_generator::pattern_random()
{
    m_idx = m_min + utils::random_uniform(m_max - m_min);
    return static_cast<off_t>(m_idx);
}

off_t pattern_generator::pattern_sequential()
{
    if (++m_idx >= m_max) { m_idx = m_min; }

    if (m_idx < m_min) m_idx = m_min;

    return m_idx;
}

off_t pattern_generator::pattern_reverse()
{
    if (--m_idx < m_min) { m_idx = m_max - 1; }

    return m_idx;
}

pattern_generator::pattern_generator()
    : m_min(0)
    , m_max(0)
    , m_idx(-1)
{
    m_generation_method = &pattern_generator::pattern_blank;
}

pattern_generator::pattern_generator(off_t p_min,
                                     off_t p_max,
                                     model::block_generation_pattern p_pattern)
{
    reset(p_min, p_max, p_pattern);
}

void pattern_generator::reset(off_t p_min,
                              off_t p_max,
                              model::block_generation_pattern p_pattern)
{
    m_min = p_min;
    m_max = p_max;
    m_idx = -1;

    const static std::unordered_map<model::block_generation_pattern,
                                    generation_method_t>
        generation_methods = {
            {model::block_generation_pattern::RANDOM,
             &pattern_generator::pattern_random},
            {model::block_generation_pattern::REVERSE,
             &pattern_generator::pattern_reverse},
            {model::block_generation_pattern::SEQUENTIAL,
             &pattern_generator::pattern_sequential},
        };

    m_generation_method = generation_methods.at(p_pattern);
}

off_t pattern_generator::generate() { return (this->*m_generation_method)(); }

} // namespace openperf::block::worker
