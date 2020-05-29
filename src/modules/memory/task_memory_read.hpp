#ifndef _OP_MEMORY_TASK_MEMORY_READ_HPP_
#define _OP_MEMORY_TASK_MEMORY_READ_HPP_

#include "memory/task_memory.hpp"

namespace openperf::memory::internal {

class task_memory_read : public openperf::memory::internal::task_memory
{
public:
    using task_memory::task_memory;

private:
    size_t operation(uint64_t nb_ops) override;
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_READ_HPP_
