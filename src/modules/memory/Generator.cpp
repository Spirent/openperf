#include "Generator.hpp"
#include "TaskConsole.hpp"

namespace openperf::memory::generator {

// Constructors & Destructor
Generator::Generator()
    : _read_threads(0)
    , _write_threads(0)
    , _runned(false)
    , _paused(false)
{

}

Generator::Generator(Generator&& g)
    : _read_threads(g._read_threads)
    , _write_threads(g._write_threads)
    , _runned(g._runned)
    , _paused(g._paused)
    , _readWorkers(std::move(g._readWorkers))
    , _writeWorkers(std::move(g._writeWorkers))
    , _readWorkerConfig(g._readWorkerConfig)
    , _writeWorkerConfig(g._writeWorkerConfig)
{

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

void Generator::setReadWorkers(unsigned int number)
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
            std::unique_ptr<Worker> worker(new Worker(_readWorkerConfig));
            worker->addTask(std::unique_ptr<TaskBase>(new TaskConsole));

            _readWorkers.push_front(std::move(worker));
        }
    }

    _read_threads = number;
}

void Generator::setWriteWorkers(unsigned int number)
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
            std::unique_ptr<Worker> worker(new Worker(_writeWorkerConfig));
            worker->addTask(std::unique_ptr<TaskBase>(new TaskConsole));

            _writeWorkers.push_front(std::move(worker));
        }
    }

    _write_threads = number;
}

void Generator::setReadWorkerConfig(const Worker::Config& config)
{
    _readWorkerConfig = config;
    for (auto& w : _readWorkers) {
       w->setConfig(_readWorkerConfig); 
    }
}

void Generator::setWriteWorkerConfig(const Worker::Config& config)
{
    _writeWorkerConfig = config;
    for (auto& w : _writeWorkers) {
       w->setConfig(_writeWorkerConfig); 
    }
}

// Methods : private
void Generator::forEachWorker(void (*callback)(WorkerPointer&))
{
    for (auto list : {&_readWorkers, &_writeWorkers}) {
        for (auto& worker : *list) { callback(worker); }
    }
}

} // namespace openperf::memory::generator
