#ifndef _OP_UTILS_WORKER_TASK_HPP_
#define _OP_UTILS_WORKER_TASK_HPP_

namespace openperf::utils::worker {

template <class T> class task
{
public:
    typedef T config_t;

public:
    virtual ~task(){};
    virtual void spin() = 0;

    virtual void config(const T&) = 0;
    virtual T config() const = 0;

    virtual void resume(){};
    virtual void pause(){};
};

} // namespace openperf::utils::worker

#endif // _OP_UTILS_WORKER_TASK_HPP_
