#ifndef _OP_MEMORY_TASK_MEMORY_WRITE_HPP_
#define _OP_MEMORY_TASK_MEMORY_WRITE_HPP_

#include "memory/task_memory.hpp"

namespace openperf::memory::internal {

class task_memory_write 
    : public task_memory
{
private:
    size_t spin(uint64_t nb_ops, size_t* op_idx) override;
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_WRITE_HPP_

