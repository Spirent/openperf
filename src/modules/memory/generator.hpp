#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include <forward_list>

#include "memory/generator_config.hpp"
#include "memory/worker.hpp"

namespace openperf::memory::internal {

class generator
{
private:
    typedef std::unique_ptr<worker> worker_ptr;
    typedef std::forward_list<worker_ptr> workers;

private:
    unsigned int _read_threads;
    unsigned int _write_threads;
    bool _runned;
    bool _paused;
    workers _read_workers;
    workers _write_workers;
    worker::config _read_worker_config;
    worker::config _write_worker_config;

public:
    // Constructors & Destructor
    generator();
    generator(generator&&);
    generator(const generator&) = delete;

    // Methods
    inline bool is_running() const { return _runned; }
    inline bool is_paused() const { return _paused; }
    inline unsigned int read_workers() const { return _read_threads; }
    inline unsigned int write_workers() const { return _write_threads; }
    inline const worker::config& read_worker_config() const
    {
        return _read_worker_config;
    }
    inline const worker::config& write_worker_config() const
    {
        return _write_worker_config;
    }

    void resume();
    void pause();

    void start();
    void stop();
    void restart();

    void set_running(bool);
    void set_read_workers(unsigned int);
    void set_read_worker_config(const worker::config&);
    void set_write_workers(unsigned int);
    void set_write_worker_config(const worker::config&);

private:
    void for_each_worker(void(worker_ptr&));
};

} // namespace openperf::memory::generator

#endif // _OP_MEMORY_GENERATOR_HPP_
