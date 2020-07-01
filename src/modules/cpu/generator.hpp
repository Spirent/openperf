#ifndef _OP_CPU_GENERATOR_HPP_
#define _OP_CPU_GENERATOR_HPP_

#include <atomic>

#include "framework/generator/controller.hpp"
#include "framework/dynamic/spool.hpp"

#include "task_cpu.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::cpu::generator {

class generator : public model::generator
{
    using controller = ::openperf::framework::generator::controller;

private:
    std::string m_result_id;
    const uint16_t m_serial_number;
    controller m_controller;

    cpu_stat m_stat;
    std::atomic<cpu_stat*> m_stat_ptr;

    dynamic::spool<cpu_stat> m_dynamic;

public:
    generator(const model::generator&);
    ~generator() override;

    void config(const generator_config&) override;
    void start();
    void start(const dynamic::configuration&);
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
