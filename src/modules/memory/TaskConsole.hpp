#ifndef _OP_MEMORY_TASKCONSOLE_HPP_
#define _OP_MEMORY_TASKCONSOLE_HPP_

#include "memory/TaskBase.hpp"
#include <iostream>
#include <ctime>

class TaskConsole : public TaskBase
{
public:
    void run() override
    {
        std::cout << "TaskConsole work " << std::hex << std::time(nullptr)
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
};

#endif // _OP_MEMORY_TASKCONSOLE_HPP_
