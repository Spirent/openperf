#ifndef _OP_MEMORY_TASK_MEMORY_WRITE_HPP_
#define _OP_MEMORY_TASK_MEMORY_WRITE_HPP_

#include "task_memory.hpp"

namespace openperf::memory::internal {

class task_memory_write : public task_memory
{
public:
    using task_memory::task_memory;

private:
    void operation(uint64_t nb_ops) override;
    memory_stat make_stat(const task_memory_stat&) override;
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_TASK_MEMORY_WRITE_HPP_
