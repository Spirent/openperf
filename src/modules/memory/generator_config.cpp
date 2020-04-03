#include "generator_config.hpp"
#include <cassert>

namespace openperf::memory {

bool generator_config::operator==(const generator_config& other) const
{
    return _buffer_size == other._buffer_size
           && _reads_per_sec == other._reads_per_sec
           && _read_size == other._read_size
           && _read_threads == other._read_threads
           && _writes_per_sec == other._writes_per_sec
           && _write_size == other._write_size
           && _write_threads == other._write_threads
           && _pattern == other._pattern;
}

generator_config& generator_config::buffer_size(int size)
{
    assert(size > 0);
    _buffer_size = size;
    return *this;
}

generator_config& generator_config::reads_per_sec(int rps)
{
    assert(rps > 0);
    _reads_per_sec = rps;
    return *this;
}

generator_config& generator_config::read_size(int size)
{
    assert(size > 0);
    _read_size = size;
    return *this;
}

generator_config& generator_config::read_threads(int number)
{
    assert(number >= 0);
    _read_threads = number;
    return *this;
}

generator_config& generator_config::writes_per_sec(int wps)
{
    assert(wps > 0);
    _writes_per_sec = wps;
    return *this;
}

generator_config& generator_config::write_size(int size)
{
    assert(size > 0);
    _write_size = size;
    return *this;
}

generator_config& generator_config::write_threads(int number)
{
    assert(number >= 0);
    _write_threads = number;
    return *this;
}

generator_config& generator_config::pattern(io_pattern pattern)
{
    _pattern = pattern;
    return *this;
}

} // namespace openperf::memory