#include "memory/generator.hpp"
#include "memory/task_memory.hpp"

namespace openperf::memory::internal {

// Constructors & Destructor
generator::generator()
    : _read_threads(0)
    , _write_threads(0)
    , _stopped(true)
    , _paused(true)
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
memory_stat generator::read_stat() const
{
    memory_stat stat;
    for (auto& ptr : _read_workers) {
        auto w = reinterpret_cast<worker<task_memory>*>(ptr.get());
        auto st = w->stat();
        stat.bytes += st.bytes;
        stat.bytes_target += st.bytes_target;
        stat.errors += st.errors;
        stat.operations += st.operations;
        stat.operations_target += st.operations_target;
        stat.time_ns += st.time_ns;
        stat.latency_min = std::min(stat.latency_min, st.latency_min);
        stat.latency_max = std::max(stat.latency_max, st.latency_max);
        stat.timestamp = st.timestamp;
    }

    return stat;
}

memory_stat generator::write_stat() const
{
    memory_stat stat;
    for (auto& ptr : _write_workers) {
        auto w = reinterpret_cast<worker<task_memory>*>(ptr.get());
        auto st = w->stat();
        stat.bytes += st.bytes;
        stat.bytes_target += st.bytes_target;
        stat.errors += st.errors;
        stat.operations += st.operations;
        stat.operations_target += st.operations_target;
        stat.time_ns += st.time_ns;
        stat.latency_min = std::min(stat.latency_min, st.latency_min);
        stat.latency_max = std::max(stat.latency_max, st.latency_max);
        stat.timestamp = st.timestamp;
    }

    return stat;
}

void generator::running(bool running)
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

    for_each_worker([](worker_ptr& w) { w->start(); });
    _stopped = false;
}

void generator::stop()
{
    if (_stopped) return;

    for_each_worker([](worker_ptr& w) { w->stop(); });
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

    for_each_worker([](worker_ptr& w) { w->resume(); });
    _paused = false;
}

void generator::pause()
{
    if (_paused) return;

    for_each_worker([](worker_ptr& w) { w->pause(); });
    _paused = true;
}

void generator::read_workers(unsigned int number)
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
            worker_ptr w(new worker<task_memory_read>());
            _read_workers.push_front(std::move(w));
        }
    }

    _read_threads = number;
}

void generator::write_workers(unsigned int number)
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
            worker_ptr w(new worker<task_memory_write>());
            _write_workers.push_front(std::move(w));
        }
    }

    _write_threads = number;
}

void generator::read_config(const task_memory_read::config_t& config)
{
    _read_config = config;
    for (auto& w : _read_workers) {
        auto wt = reinterpret_cast<worker<task_memory>*>(w.get());
        wt->config(_read_config);
    }
}

void generator::write_config(const task_memory_write::config_t& config)
{
    _write_config = config;
    for (auto& w : _write_workers) {
        auto wt = reinterpret_cast<worker<task_memory>*>(w.get());
        wt->config(_write_config);
    }
}

void generator::clear_stat()
{
    for_each_worker([](worker_ptr& w){
        auto wtm = reinterpret_cast<worker<task_memory>*>(w.get());
        wtm->clear_stat();
    });
}

// Methods : private
void generator::for_each_worker(std::function<void(worker_ptr&)> function)
{
    for (auto list : {&_read_workers, &_write_workers}) {
        for (auto& worker : *list) { function(worker); }
    }
}

} // namespace openperf::memory::internal
