#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include <forward_list>

#include "memory/GeneratorConfig.hpp"
#include "memory/Worker.hpp"

namespace openperf::memory::generator {

class Generator
{
private:
    typedef std::unique_ptr<Worker> WorkerPointer;
    typedef std::forward_list<WorkerPointer> WorkerList;

private:
    unsigned int _read_threads;
    unsigned int _write_threads;
    bool _runned;
    bool _paused;
    WorkerList _readWorkers;
    WorkerList _writeWorkers;
    Worker::Config _readWorkerConfig;
    Worker::Config _writeWorkerConfig;

public:
    // Constructors & Destructor
    Generator();
    Generator(Generator&&);
    Generator(const Generator&) = delete;

    // Methods
    inline bool isRunning() const { return _runned; }
    inline bool isPaused() const { return _paused; }
    inline unsigned int readWorkers() const { return _read_threads; }
    inline unsigned int writeWorkers() const { return _write_threads; }
    inline const Worker::Config& readWorkerConfig() const
    {
        return _readWorkerConfig;
    }
    inline const Worker::Config& writeWorkerConfig() const
    {
        return _writeWorkerConfig;
    }

    void resume();
    void pause();

    void start();
    void stop();
    void restart();

    void setRunning(bool);
    void setReadWorkers(unsigned int);
    void setReadWorkerConfig(const Worker::Config&);
    void setWriteWorkers(unsigned int);
    void setWriteWorkerConfig(const Worker::Config&);

private:
    void forEachWorker(void(WorkerPointer&));
};

} // namespace openperf::memory::generator

#endif // _OP_MEMORY_GENERATOR_HPP_
