#ifndef _OP_NETWORK_GENERATOR_HPP_
#define _OP_NETWORK_GENERATOR_HPP_

#include <atomic>

#include "framework/generator/controller.hpp"
#include "modules/dynamic/spool.hpp"

#include "models/generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::network::internal {

class generator : public model::generator
{
    using controller = ::openperf::framework::generator::controller;
    using chronometer = openperf::timesync::chrono::realtime;
    using generator_stat_t = model::generator_result::statistics_t;

private:
    std::string m_result_id;
    const uint16_t m_serial_number;

    generator_stat_t m_read_stat, m_write_stat;
    std::atomic<generator_stat_t*> m_read_stat_ptr, m_write_stat_ptr;
    chronometer::time_point m_start_time;

    dynamic::spool<model::generator_result> m_dynamic;
    controller m_controller;

public:
    generator(const model::generator&);
    ~generator() override;

    void config(const model::generator_config&) override;
    void start();
    void start(const dynamic::configuration&);
    void stop();
    void reset();

    void running(bool) override;
    bool running() const override { return model::generator::running(); }

    model::generator_result statistics() const;
};

} // namespace openperf::network::internal

#endif // _OP_CPU_GENERATOR_HPP_
