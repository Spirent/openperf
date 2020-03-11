#ifndef _OP_BLOCK_GENERATOR_WORKER_TASK_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_TASK_HPP_

namespace openperf::block::worker {

enum block_task_type {
    READER = (1 << 0),
    WRITER = (1 << 1)
};

class worker_task {
private:
    block_task_type type;
public:
    worker_task(block_task_type p_type) {
        type = p_type;
    };
    virtual ~worker_task() = 0;
    virtual void run() = 0;
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_TASK_HPP_