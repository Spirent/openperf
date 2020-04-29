#ifndef _OP_CPU_INFO_MODEL_HPP_
#define _OP_CPU_INFO_MODEL_HPP_

#include <string>

namespace openperf::cpu::model {

class cpu_info
{
public:
    cpu_info() = default;
    cpu_info(const cpu_info&) = default;

    inline int32_t cores() const { return m_cores; }
    inline void cores(int32_t cores) { m_cores = cores; }

    inline int64_t cache_line_size() const { return m_cache_line_size; }
    inline void cache_line_size(int64_t size) { m_cache_line_size = size; }

    inline std::string architecture() const { return m_architecture; }
    inline void architecture(std::string_view arch) { m_architecture = arch; }

protected:
    int32_t m_cores;
    int64_t m_cache_line_size;
    std::string m_architecture;
};

} // namespace openperf::cpu::model

#endif // _OP_CPU_INFO_MODEL_HPP_