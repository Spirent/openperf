#ifndef _OP_TASK_BASE_HPP_
#define _OP_TASK_BASE_HPP_

class TaskBase
{
public:
    virtual ~TaskBase(){};
    virtual void run() = 0;
};

#endif // _OP_TASK_BASE_HPP_
