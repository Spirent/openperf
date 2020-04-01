#ifndef _MEMORY_INFO_HPP_
#define _MEMORY_INFO_HPP_

#include "swagger/v1/model/MemoryInfoResult.h"

namespace openperf::memory {

namespace model = swagger::v1::model;

class memory_info
{
public:
    memory_info() = delete;

    static model::MemoryInfoResult get();
};

} // namespace openperf::memory

#endif // _MEMORY_INFO_HPP_
