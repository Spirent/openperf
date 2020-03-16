#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include "GeneratorConfig.hpp"
#include "Worker.hpp"
#include <forward_list>

namespace openperf::memory::generator {

class Generator
{
public:
    struct Config
    {
        bool running;
        int read_threads;
        int write_threads;
    };

private:
    typedef std::unique_ptr<Worker> WorkerPointer;
    typedef std::forward_list<WorkerPointer> WorkerList;

private:
    int _read_threads;
    int _write_threads;
    bool _runned;
    bool _paused;
    WorkerList _readWorkers;
    WorkerList _writeWorkers;

public:
    // Constructors & Destructor
    Generator(const Generator::Config&);
    Generator(const Generator&) = delete;

    // Methods
    const Config& config() const;
    inline bool isRunning() const { return _runned; }
    inline bool isPaused() const { return _paused; }
    inline int readThreads() const { return _read_threads; }
    inline int writeThreads() const { return _write_threads; }

    void resume();
    void pause();

    void start();
    void stop();
    void restart();

    void setRunning(bool);
    void setReadThreads(int);
    void setWriteThreads(int);

private:
    void forEachWorker(void(WorkerPointer&));
};

} // namespace openperf::memory::generator

#endif // _OP_MEMORY_GENERATOR_HPP_
