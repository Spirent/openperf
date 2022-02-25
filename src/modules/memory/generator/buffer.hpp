#ifndef _OP_MEMORY_GENERATOR_BUFFER_HPP_
#define _OP_MEMORY_GENERATOR_BUFFER_HPP_

#include <cstddef>

namespace openperf::memory::generator {

class buffer
{
    std::byte* m_data;
    size_t m_length;

public:
    buffer(size_t size);
    ~buffer();

    std::byte* data();
    const std::byte* data() const;

    size_t length() const;
};

} // namespace openperf::memory::generator

#endif /* _OP_MEMORY_GENERATOR_BUFFER_HPP_ */
