#ifndef _OP_CPU_INFO_HPP_
#define _OP_CPU_INFO_HPP_

#include <unistd.h>
#include <thread>

#ifndef ARCH
#define ARCH "unknown"
#endif

namespace openperf::cpu::info {

int32_t cores_count();
int64_t cache_line_size();
std::string architecture();

} // namespace openperf::cpu::info

#endif // _OP_CPU_INFO_HPP_
