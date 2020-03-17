#ifndef _OP_MEMORY_GENERATORCONFIG_HPP_
#define _OP_MEMORY_GENERATORCONFIG_HPP_

namespace openperf::memory {

class GeneratorConfig
{
private:
    bool _running;
    unsigned int _buffer_size;
    unsigned int _reads_per_sec;
    unsigned int _read_size;
    unsigned int _read_threads;
    unsigned int _writes_per_sec;
    unsigned int _write_size;
    unsigned int _write_threads;

public:
    inline static GeneratorConfig create() { return GeneratorConfig(); }

    inline bool isRunning() const { return _running; }
    inline unsigned int bufferSize() const { return _buffer_size; }
    inline unsigned int readsPerSec() const { return _reads_per_sec; }
    inline unsigned int readSize() const { return _read_size; }
    inline unsigned int readThreads() const { return _read_threads; }
    inline unsigned int writesPerSec() const { return _writes_per_sec; }
    inline unsigned int writeSize() const { return _write_size; }
    inline unsigned int writeThreads() const { return _write_threads; }

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
