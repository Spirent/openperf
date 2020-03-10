#ifndef _MEMORY_INFO_HPP_
#define _MEMORY_INFO_HPP_

#include "swagger/v1/model/MemoryInfoResult.h"

namespace openperf::memory::info {

using namespace swagger::v1::model;

typedef std::shared_ptr<MemoryInfoResult> MemoryInfoResultPointer;

class memory_info
{
public:
    MemoryInfoResultPointer get() const;
};

} // namespace openperf::memory::info

#endif // _MEMORY_INFO_HPP_