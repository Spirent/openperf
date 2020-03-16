#include "Generator.hpp"
#include "TaskConsole.hpp"

namespace openperf::memory::generator {

// Constructors & Destructor
Generator::Generator(const Generator::Config& config)
    : _read_threads(0)
    , _write_threads(0)
    , _runned(false)
    , _paused(false)
{
    setReadThreads(config.read_threads);
    setWriteThreads(config.write_threads);
    setRunning(config.running);
}

// Methods : public
void Generator::setRunning(bool running)
{
    if (running)
        start();
    else
        stop();
}

void Generator::start()
{
    if (_runned) return;

    std::cout << "Generator::start()" << std::endl;
    forEachWorker([](auto w) { w->start(); });

    _runned = true;
}

void Generator::stop()
{
    if (!_runned) return;

    std::cout << "Generator::stop()" << std::endl;
    forEachWorker([](auto w) { w->stop(); });

    _runned = false;
}

void Generator::restart()
{
    stop();
    start();
}

void Generator::resume()
{
    if (!_paused) return;

    std::cout << "Generator::resume()" << std::endl;
    forEachWorker([](auto w) { w->resume(); });

    _paused = false;
}

void Generator::pause()
{
    if (_paused) return;

    std::cout << "Generator::pause()" << std::endl;
    forEachWorker([](auto w) { w->pause(); });

    _paused = true;
}

void Generator::setReadThreads(int number)
{
    assert(number >= 0);
    if (number == 0) {
        _readWorkers.clear();
        _read_threads = 0;
        return;
    }

    if (number < _read_threads) {
        for (; _read_threads > number; --_read_threads) {
            _readWorkers.pop_front();
        }
    } else {
        for (; _read_threads < number; ++_read_threads) {
            std::unique_ptr<Worker> worker(new Worker);
            worker->addTask(std::unique_ptr<TaskBase>(new TaskConsole));

            _readWorkers.push_front(std::move(worker));
        }
    }

    _read_threads = number;
}

void Generator::setWriteThreads(int number)
{
    assert(number >= 0);
    if (number == 0) {
        _writeWorkers.clear();
        _write_threads = 0;
        return;
    }

    if (number < _write_threads) {
        for (; _write_threads > number; --_write_threads) {
            _writeWorkers.pop_front();
        }
    } else {
        for (; _write_threads < number; ++_write_threads) {
            std::unique_ptr<Worker> worker(new Worker);
            worker->addTask(std::unique_ptr<TaskBase>(new TaskConsole));

            _writeWorkers.push_front(std::move(worker));
        }
    }

    _write_threads = number;
}

void Generator::forEachWorker(void (*callback)(WorkerPointer&))
{
    for (auto list : {&_readWorkers, &_writeWorkers}) {
        for (auto& worker : *list) { callback(worker); }
    }
}

} // namespace openperf::memory::generator
