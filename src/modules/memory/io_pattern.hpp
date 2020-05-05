#ifndef _OP_MEMORY_IO_PATTERN_HPP_
#define _OP_MEMORY_IO_PATTERN_HPP_

namespace openperf::memory::internal {

enum class io_pattern {
    NONE = 0,
    SEQUENTIAL,
    REVERSE,
    RANDOM,
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_IO_PATTERN_HPP_
