#ifndef _OP_MEMORY_WORKER_INTERFACE_HPP_
#define _OP_MEMORY_WORKER_INTERFACE_HPP_

namespace openperf::memory::internal {

class worker_interface
{
public:
    virtual ~worker_interface() {}

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;

    virtual bool is_paused() const = 0;
    virtual bool is_running() const = 0;
    virtual bool is_finished() const = 0;
};

} // namespace openperf::memory::internal 

#endif // _OP_MEMORY_WORKER_INTERFACE_HPP_
