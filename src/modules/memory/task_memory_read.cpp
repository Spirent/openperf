#include "task_memory_read.hpp"

#include <cassert>
#include <cstring>

namespace openperf::memory::internal {

void task_memory_read::operation(uint64_t nb_ops)
{
    auto indexes = m_config.indexes.get();
    auto* buffer_pointer = reinterpret_cast<uint8_t*>(m_buffer->data());

    assert(m_op_index < indexes.size());
    assert(buffer_pointer != nullptr);

    for (size_t i = 0; i < nb_ops; ++i) {
        auto idx = indexes.at(m_op_index++);
        std::memcpy(m_scratch.data(),
                    buffer_pointer + (idx * m_config.block_size),
                    m_config.block_size);
        if (m_op_index == indexes.size()) { m_op_index = 0; }
    }
}

memory_stat task_memory_read::make_stat(const task_memory_stat& stat)
{
    return {.read = stat};
}

} // namespace openperf::memory::internal
