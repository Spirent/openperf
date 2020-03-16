#ifndef _OP_MEMORY_GENERATORCONFIG_HPP_
#define _OP_MEMORY_GENERATORCONFIG_HPP_

namespace openperf::memory {

class GeneratorConfig
{
private:
    bool _running;
    int _buffer_size;
    int _reads_per_sec;
    int _read_size;
    int _read_threads;
    int _writes_per_sec;
    int _write_size;
    int _write_threads;

public:
    inline static GeneratorConfig create() { return GeneratorConfig(); }

    inline int isRunning() const { return _running; }
    inline int bufferSize() const { return _buffer_size; }
    inline int readsPerSec() const { return _reads_per_sec; }
    inline int readSize() const { return _read_size; }
    inline int readThreads() const { return _read_threads; }
    inline int writesPerSec() const { return _writes_per_sec; }
    inline int writeSize() const { return _write_size; }
    inline int writeThreads() const { return _write_threads; }

    bool operator==(const GeneratorConfig&) const;

    GeneratorConfig& setRunning(bool);
    GeneratorConfig& setBufferSize(int);
    GeneratorConfig& setReadsPerSec(int);
    GeneratorConfig& setReadSize(int);
    GeneratorConfig& setReadThreads(int);
    GeneratorConfig& setWritesPerSec(int);
    GeneratorConfig& setWriteSize(int);
    GeneratorConfig& setWriteThreads(int);
};

} // namespace openperf::memory

#endif // _OP_MEMORY_GENERATORCONFIG_HPP_
