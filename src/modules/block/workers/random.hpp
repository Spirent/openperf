#ifndef _OP_BLOCK_GENERATOR_WORKER_RANDOM_HPP_
#define _OP_BLOCK_GENERATOR_WORKER_RANDOM_HPP_

#include "block/worker_task.hpp"
#include <stdio.h>

namespace openperf::block::worker {

class worker_task_random : public worker_task {
    
public:
    worker_task_random(){};
    ~worker_task_random(){};
    void read() {
        printf("read\n");
    };
    void write() {
        printf("write\n");
    };
};

} // namespace openperf::block::worker

#endif // _OP_BLOCK_GENERATOR_WORKER_RANDOM_HPP_