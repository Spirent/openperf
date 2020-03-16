#include "GeneratorConfig.hpp"
#include <cassert>

namespace openperf::memory {

bool GeneratorConfig::operator==(const GeneratorConfig& other) const
{
    return _running == other._running && _buffer_size == other._buffer_size
           && _reads_per_sec == other._reads_per_sec
           && _read_size == other._read_size
           && _read_threads == other._read_threads
           && _writes_per_sec == other._writes_per_sec
           && _write_size == other._write_size
           && _write_threads == other._write_threads;
}

GeneratorConfig& GeneratorConfig::setRunning(bool running)
{
    _running = running;
    return *this;
}

GeneratorConfig& GeneratorConfig::setBufferSize(int size)
{
    assert(size > 0);
    _buffer_size = size;
    return *this;
}

GeneratorConfig& GeneratorConfig::setReadsPerSec(int rps)
{
    assert(rps > 0);
    _reads_per_sec = rps;
    return *this;
}

GeneratorConfig& GeneratorConfig::setReadSize(int size)
{
    assert(size > 0);
    _read_size = size;
    return *this;
}

GeneratorConfig& GeneratorConfig::setReadThreads(int number)
{
    assert(number > 0);
    _read_threads = number;
    return *this;
}

GeneratorConfig& GeneratorConfig::setWritesPerSec(int wps)
{
    assert(wps > 0);
    _writes_per_sec = wps;
    return *this;
}

GeneratorConfig& GeneratorConfig::setWriteSize(int size)
{
    assert(size > 0);
    _write_size = size;
    return *this;
}

GeneratorConfig& GeneratorConfig::setWriteThreads(int number)
{
    assert(number > 0);
    _write_threads = number;
    return *this;
}

} // namespace openperf::memory