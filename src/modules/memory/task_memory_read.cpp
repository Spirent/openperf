#include "memory/task_memory_read.hpp"

#include <cassert>
#include <cstring>

namespace openperf::memory::internal {

size_t task_memory_read::operation(uint64_t nb_ops, size_t* op_idx)
{
    assert(*op_idx < m_indexes.size());
    for (size_t i = 0; i < nb_ops; ++i) {
        unsigned idx = m_indexes.at((*op_idx)++);
        std::memcpy(m_scratch.ptr,
                    m_buffer + (idx * m_config.block_size),
                    m_config.block_size);
        if (*op_idx == m_indexes.size()) { *op_idx = 0; }
    }

    return nb_ops;
}

} // namespace openperf::memory::internal
