#ifndef _OP_MEMORY_TASK_MEMORY_WRITE_HPP_
#define _OP_MEMORY_TASK_MEMORY_WRITE_HPP_

#include "memory/task_memory.hpp"

namespace openperf::memory::internal {

class task_memory_write : public task_memory
{
public:
    using task_memory::task_memory;

    memory_stat spin() override;

private:
    void operation(uint64_t nb_ops) override;
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_WRITE_HPP_
