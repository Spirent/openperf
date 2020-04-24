#ifndef _OP_CPU_GENERATOR_MODEL_HPP_
#define _OP_CPU_GENERATOR_MODEL_HPP_

#include <string>
#include <vector>

namespace openperf::cpu::model {

enum class cpu_instruction_set { SCALAR };
enum class cpu_operation { INT, FLOAT };

struct cpu_generator_target_config
{
    cpu_instruction_set instruction_set;
    uint data_size;
    cpu_operation operation;
    uint weight;
};

struct cpu_generator_core_config
{
    double utilization;
    std::vector<cpu_generator_target_config> targets;
};

struct cpu_generator_config
{
    std::vector<cpu_generator_core_config> cores;
};

class cpu_generator
{
public:
    cpu_generator() = default;
    cpu_generator(const cpu_generator&) = default;
    virtual ~cpu_generator() = default;

    virtual std::string get_id() const;
    virtual void set_id(std::string_view value);

    virtual cpu_generator_config get_config() const;
    virtual void set_config(const cpu_generator_config& value);

    virtual bool is_running() const;
    virtual void set_running(const bool value);

protected:
    std::string m_id;
    cpu_generator_config m_config;
    bool m_running;
};

} // namespace openperf::cpu::model

#endif // _OP_CPU_GENERATOR_MODEL_HPP_