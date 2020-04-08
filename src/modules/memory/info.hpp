#ifndef _MEMORY_INFO_HPP_
#define _MEMORY_INFO_HPP_

#include <cinttypes>

namespace openperf::memory {

class memory_info
{
public:
    struct info_t
    {
        uint64_t total;
        uint64_t free;
    };

public:
    memory_info() = delete;

    static info_t get();
};

} // namespace openperf::memory

#endif // _MEMORY_INFO_HPP_
