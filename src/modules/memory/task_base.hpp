#ifndef _OP_TASK_BASE_HPP_
#define _OP_TASK_BASE_HPP_

class task_base 
{
public:
    virtual ~task_base() {};

    virtual void run() = 0;
};

#endif // _OP_TASK_BASE_HPP_
