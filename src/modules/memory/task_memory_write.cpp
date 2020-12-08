#include "task_memory_write.hpp"

#include <cassert>
#include <cstring>

namespace openperf::memory::internal {

void task_memory_write::operation(uint64_t nb_ops)
{
    assert(m_op_index < m_config.indexes->size());

    auto* buffer_pointer = reinterpret_cast<uint8_t*>(m_buffer->data());
    for (size_t i = 0; i < nb_ops; ++i) {
        auto idx = m_config.indexes->at(m_op_index++);
        std::memcpy(buffer_pointer + (idx * m_config.block_size),
                    m_scratch.data(),
                    m_config.block_size);
        if (m_op_index == m_config.indexes->size()) { m_op_index = 0; }
    }
}

memory_stat task_memory_write::make_stat(const task_memory_stat& stat)
{
    return {.write = stat};
}

} // namespace openperf::memory::internal
