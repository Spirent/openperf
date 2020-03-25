#include "memory/task_memory_write.hpp"

#include <cassert>
#include <cstring>

namespace openperf::memory::internal {

size_t task_memory_write::operation(uint64_t nb_ops, size_t* op_idx)
{
    assert(*op_idx < _op_index_max);
    for (size_t i = 0; i < nb_ops; i++) {
        // unsigned idx = op_packed_array_get(_config.indexes, (*op_idx)++);
        unsigned idx = _indexes.at((*op_idx)++);
        std::memcpy(_buffer + (idx * _config.block_size),
                    _scratch_buffer,
                    _config.block_size);
        if (*op_idx == _op_index_max) { *op_idx = _op_index_min; }
    }

    return nb_ops;
}

} // namespace openperf::memory::internal
