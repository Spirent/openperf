#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include <forward_list>

#include "utils/worker/worker.hpp"
#include "memory/generator_config.hpp"
#include "memory/task_memory_read.hpp"
#include "memory/task_memory_write.hpp"

namespace openperf::memory::internal {

using namespace openperf::utils::worker;

class generator
{
private:
    typedef std::unique_ptr<workable> worker_ptr;
    typedef std::forward_list<worker_ptr> workers;

private:
    unsigned _read_threads;
    unsigned _write_threads;
    bool _stopped;
    bool _paused;
    workers _read_workers;
    workers _write_workers;
    task_memory_config _read_config;
    task_memory_config _write_config;

public:
    // Constructors & Destructor
    generator();
    generator(generator&&);
    generator(const generator&) = delete;

    // Methods
    inline bool is_stopped() const { return _stopped; }
    inline bool is_running() const { return !(_paused || _stopped); }
    inline bool is_paused() const { return _paused; }
    inline unsigned read_workers() const { return _read_threads; }
    inline unsigned write_workers() const { return _write_threads; }
    inline const task_memory_config& read_worker_config() const
    {
        return _read_config;
    }
    inline const task_memory_config& write_worker_config() const
    {
        return _write_config;
    }
    memory_stat read_stat() const;
    memory_stat write_stat() const;

    void resume();
    void pause();

    void start();
    void stop();
    void restart();
    void clear_stat();

    void running(bool);
    void read_workers(unsigned);
    void write_workers(unsigned);
    void read_config(const task_memory_config&);
    void write_config(const task_memory_config&);

private:
    void for_each_worker(std::function<void(worker_ptr&)>);
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_GENERATOR_HPP_
