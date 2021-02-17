#ifndef _OP_FRAMEWORK_GENERATOR_TASK_HPP_
#define _OP_FRAMEWORK_GENERATOR_TASK_HPP_

namespace openperf::framework::generator {

class task_base
{
public:
    virtual ~task_base() = default;
};

template <typename S> class task : public task_base
{
public:
    virtual ~task() = default;

    virtual S spin() = 0;
    virtual void reset() = 0;
};

} // namespace openperf::framework::generator

#endif // _OP_FRAMEWORK_GENERATOR_TASK_HPP_
