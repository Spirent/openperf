#ifndef _OP_CPU_GENERATOR_HPP_
#define _OP_CPU_GENERATOR_HPP_

#include <atomic>

#include "framework/generator/controller.hpp"

#include "models/generator.hpp"
#include "models/generator_result.hpp"
#include "task_cpu.hpp"

namespace openperf::cpu::generator {

using namespace openperf::cpu::internal;

class generator : public model::generator
{
    using controller = ::openperf::framework::generator::controller;

private:
    std::string m_result_id;
    const uint16_t m_serial_number;
    controller m_controller;

    cpu_stat m_stat;
    std::atomic<cpu_stat*> m_stat_ptr;

public:
    generator(const model::generator&);
    ~generator() override;

    void config(const model::generator_config&) override;
    void start();
    void stop();
    void reset();

    void running(bool) override;
    bool running() const override { return model::generator::running(); }

    model::generator_result statistics() const;

private:
    bool is_supported(cpu::instruction_set);
};

} // namespace openperf::cpu::generator

#endif // _OP_CPU_GENERATOR_HPP_
