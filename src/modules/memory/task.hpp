#ifndef _OP_TASK_HPP_
#define _OP_TASK_HPP_

namespace openperf::generator::generic {

class task
{
public:
    virtual ~task(){};
    virtual void run() = 0;
};

} // namespace openperf::generator::generic

#endif // _OP_TASK_HPP_
