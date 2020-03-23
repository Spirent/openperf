#include "memory/generator.hpp"
#include "memory/task.hpp"
#include "memory/task_console.hpp"
#include "memory/task_memory.hpp"

namespace openperf::memory::internal {

using namespace openperf::generator::generic;

// Constructors & Destructor
generator::generator()
    : _read_threads(0)
    , _write_threads(0)
    , _stopped(false)
    , _paused(false)
{}

generator::generator(generator&& g)
    : _read_threads(g._read_threads)
    , _write_threads(g._write_threads)
    , _stopped(g._stopped)
    , _paused(g._paused)
    , _read_workers(std::move(g._read_workers))
    , _write_workers(std::move(g._write_workers))
    , _read_config(g._read_config)
    , _write_config(g._write_config)
{}

// Methods : public
void generator::set_running(bool running)
{
    if (running) {
        start();
        resume();
    } else {
        pause();
    }
}

void generator::start()
{
    if (!_stopped) return;

    std::cout << "generator::start()" << std::endl;
    for_each_worker([](auto w) { w->start(); });
    _stopped = false;
}

void generator::stop()
{
    if (_stopped) return;

    std::cout << "generator::stop()" << std::endl;
    for_each_worker([](auto w) { w->stop(); });
    _stopped = false;
}

void generator::restart()
{
    stop();
    start();
}

void generator::resume()
{
    if (!_paused) return;

    std::cout << "generator::resume()" << std::endl;
    for_each_worker([](auto w) { w->resume(); });
    _paused = false;
}

void generator::pause()
{
    if (_paused) return;

    std::cout << "generator::pause()" << std::endl;
    for_each_worker([](auto w) { w->pause(); });
    _paused = true;
}

void generator::set_read_workers(unsigned int number)
{
    assert(number >= 0);
    if (number == 0) {
        _read_workers.clear();
        _read_threads = 0;
        return;
    }

    if (number < _read_threads) {
        for (; _read_threads > number; --_read_threads) {
            _read_workers.pop_front();
        }
    } else {
        for (; _read_threads < number; ++_read_threads) {
            std::unique_ptr<worker> w(new worker);
            w->add_task(std::unique_ptr<task_memory>(new task_memory_read));
            w->add_task(std::unique_ptr<task>(new task_console("worker read")));

            _read_workers.push_front(std::move(w));
        }
    }

    _read_threads = number;
}

void generator::set_write_workers(unsigned int number)
{
    assert(number >= 0);
    if (number == 0) {
        _write_workers.clear();
        _write_threads = 0;
        return;
    }

    if (number < _write_threads) {
        for (; _write_threads > number; --_write_threads) {
            _write_workers.pop_front();
        }
    } else {
        for (; _write_threads < number; ++_write_threads) {
            std::unique_ptr<worker> w(new worker);
            w->add_task(std::unique_ptr<task_memory>(new task_memory_write));
            w->add_task(
                std::unique_ptr<task>(new task_console("worker write")));

            _write_workers.push_front(std::move(w));
        }
    }

    _write_threads = number;
}

void generator::set_read_config(const task_memory_read::config& config)
{
    _read_config = config;
    for (auto& w : _read_workers) { 
        w->set_config<task_memory_read>(_read_config);
    }
}

void generator::set_write_config(const task_memory_write::config& config)
{
    _write_config = config;
    for (auto& w : _write_workers) { 
        w->set_config<task_memory_write>(_write_config); 
    }
}

// Methods : private
void generator::for_each_worker(void (*callback)(worker_ptr&))
{
    for (auto list : {&_read_workers, &_write_workers}) {
        for (auto& worker : *list) { callback(worker); }
    }
}

} // namespace openperf::memory::internal
