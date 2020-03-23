#ifndef _OP_MEMORY_TASK_CONSOLE_HPP_
#define _OP_MEMORY_TASK_CONSOLE_HPP_

#include "memory/task.hpp"
#include <iostream>
#include <ctime>

namespace openperf::generator::generic {

class task_console 
    : public openperf::generator::generic::task
{
public:
    void run() override
    {
        std::cout << "Task Console work " << std::hex << std::time(nullptr)
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
};

} // namespace openperf::generator::generic

#endif // _OP_MEMORY_TASK_CONSOLE_HPP_
