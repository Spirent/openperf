#ifndef _OP_BLOCK_GENERATOR_WORKER_TASK_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_TASK_HPP_

namespace openperf::block::worker {

class worker_task {
public:
    virtual ~worker_task(){};
    virtual void read() = 0;
    virtual void write() = 0;
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_TASK_HPP_