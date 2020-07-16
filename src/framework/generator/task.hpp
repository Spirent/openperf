#ifndef _OP_UTILS_WORKER_TASK_HPP_
#define _OP_UTILS_WORKER_TASK_HPP_

namespace openperf::framework::generator {

template <typename S> class task
{
public:
    virtual ~task() = default;

    virtual S spin() = 0;
    virtual void reset() = 0;

    virtual void resume(){};
    virtual void pause(){};
};

} // namespace openperf::generator

#endif // _OP_UTILS_WORKER_TASK_HPP_
