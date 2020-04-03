#ifndef _OP_MEMORY_GENERATOR_CONFIG_HPP_
#define _OP_MEMORY_GENERATOR_CONFIG_HPP_

#include <cinttypes>
#include "memory/io_pattern.hpp"

namespace openperf::memory {

class generator_config
{
private:
    unsigned int _buffer_size;
    unsigned int _reads_per_sec;
    unsigned int _read_size;
    unsigned int _read_threads;
    unsigned int _writes_per_sec;
    unsigned int _write_size;
    unsigned int _write_threads;
    io_pattern _pattern;

public:
    inline static generator_config create() { return generator_config(); }

    inline unsigned int buffer_size() const { return _buffer_size; }
    inline unsigned int reads_per_sec() const { return _reads_per_sec; }
    inline unsigned int read_size() const { return _read_size; }
    inline unsigned int read_threads() const { return _read_threads; }
    inline unsigned int writes_per_sec() const { return _writes_per_sec; }
    inline unsigned int write_size() const { return _write_size; }
    inline unsigned int write_threads() const { return _write_threads; }
    inline io_pattern pattern() const { return _pattern; }

    bool operator==(const generator_config&) const;

    generator_config& buffer_size(int);
    generator_config& reads_per_sec(int);
    generator_config& read_size(int);
    generator_config& read_threads(int);
    generator_config& writes_per_sec(int);
    generator_config& write_size(int);
    generator_config& write_threads(int);
    generator_config& pattern(io_pattern);
};

} // namespace openperf::memory

#endif // _OP_MEMORY_GENERATOR_CONFIG_HPP_
