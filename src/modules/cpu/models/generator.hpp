#ifndef _OP_CPU_GENERATOR_MODEL_HPP_
#define _OP_CPU_GENERATOR_MODEL_HPP_

#include <string>
#include <vector>
#include "cpu/common.hpp"

namespace openperf::cpu::model {

struct generator_target_config
{
    cpu::instruction_set instruction_set;
    cpu::operation operation;
    uint64_t data_size;
    uint64_t weight;
};

struct generator_core_config
{
    double utilization;
    std::vector<generator_target_config> targets;
};

struct generator_config
{
    std::vector<generator_core_config> cores;
};

class generator
{
public:
    generator() = default;
    generator(const generator&) = default;
    virtual ~generator() = default;

    inline virtual std::string id() const { return m_id; }
    inline virtual void id(std::string_view id) { m_id = id; }

    inline virtual generator_config config() const { return m_config; }
    inline virtual void config(const generator_config& value) { m_config = value; }

    inline virtual bool running() const { return m_running; }
    inline virtual void running(bool value) { m_running = value; }

protected:
    std::string m_id;
    generator_config m_config;
    bool m_running;
};

} // namespace openperf::cpu::model

#endif // _OP_CPU_GENERATOR_MODEL_HPP_
