#ifndef _OP_MEMORY_TASK_CONSOLE_HPP_
#define _OP_MEMORY_TASK_CONSOLE_HPP_

#include "memory/task.hpp"
#include <iostream>
#include <ctime>

namespace openperf::generator::generic {

class task_console : public openperf::generator::generic::task
{
private:
    std::string _msg;

public:
    task_console() {}
    task_console(const std::string& msg)
        : _msg(msg)
    {}

    void run() override
    {
        std::cout << "[" << std::hex << std::this_thread::get_id()
                  << "]: " << std::dec << std::time(nullptr)
                  << " > task_console::run()" << std::endl;
        if (!_msg.empty()) { std::cout << " * " << _msg << std::endl; }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
};

} // namespace openperf::generator::generic

#endif // _OP_MEMORY_TASK_CONSOLE_HPP_
