#ifndef _OP_CPU_GENERATOR_HPP_
#define _OP_CPU_GENERATOR_HPP_

#include <atomic>

#include "framework/generator/controller.hpp"
#include "modules/dynamic/spool.hpp"

#include "task_cpu.hpp"
#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::cpu::generator {

class generator : public model::generator
{
    using controller = ::openperf::framework::generator::controller;
    using chronometer = openperf::timesync::chrono::realtime;

private:
    std::string m_result_id;
    const uint16_t m_serial_number;

    cpu_stat m_stat;
    std::atomic<cpu_stat*> m_stat_ptr;
    chronometer::time_point m_start_time;

    dynamic::spool<cpu_stat> m_dynamic;
    controller m_controller;

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
};

} // namespace openperf::cpu::generator

#endif // _OP_CPU_GENERATOR_HPP_
