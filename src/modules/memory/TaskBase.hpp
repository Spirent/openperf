#ifndef _OP_TASK_BASE_HPP_
#define _OP_TASK_BASE_HPP_

#include <list>

class TaskBase
{
public:
    virtual ~TaskBase(){};
    virtual void run() = 0;
};

typedef std::list<std::unique_ptr<TaskBase>> TaskList;

#endif // _OP_TASK_BASE_HPP_
