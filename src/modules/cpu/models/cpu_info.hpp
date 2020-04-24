#ifndef _OP_CPU_INFO_MODEL_HPP_
#define _OP_CPU_INFO_MODEL_HPP_

#include <string>

namespace openperf::cpu::model {

class cpu_info
{
public:
    cpu_info() = default;
    cpu_info(const cpu_info&) = default;

    int32_t get_cores() const;
    void set_cores(int32_t);

    int64_t get_cache_line_size() const;
    void set_cache_line_size(int64_t);

    std::string get_architecture() const;
    void set_architecture(std::string_view value);

protected:
    int32_t m_cores;
    int64_t m_cache_line_size;
    std::string m_architecture;
};

} // namespace openperf::cpu::model

#endif // _OP_CPU_INFO_MODEL_HPP_