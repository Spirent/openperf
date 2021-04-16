#ifndef _OP_NETWORK_GENERATOR_HPP_
#define _OP_NETWORK_GENERATOR_HPP_

#include <atomic>

#include "framework/generator/controller.hpp"
#include "network/models/generator.hpp"
#include "network/models/generator_result.hpp"
#include "modules/dynamic/spool.hpp"
#include "task.hpp"

namespace openperf::network {

namespace model {
class generator_result;
} // namespace model

namespace internal {

class generator;

class stat_collector
{
    generator* m_gen;

public:
    stat_collector(generator* gen);
    void set_generator(generator* gen);

    void update(const task::stat_t& stat);
};

class generator : public model::generator
{
    friend class stat_collector;
    using controller = ::openperf::framework::generator::controller;
    using chronometer = openperf::timesync::chrono::realtime;
    using generator_stat_t = model::generator_result::load_stat_t;

private:
    std::string m_result_id;
    uint16_t m_serial_number;

    task::synchronizer_t m_synchronizer;

    task::stat_t m_read_stat, m_write_stat;
    task::conn_stat_t m_conn_stat;
    chronometer::time_point m_start_time;

    dynamic::spool<model::generator_result> m_dynamic;
    std::unique_ptr<stat_collector> m_stat_collector;
    std::unique_ptr<controller> m_controller;

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

    friend void swap_running(generator& out_gen,
                             generator& in_gen,
                             const dynamic::configuration& cfg);
};

} // namespace internal
} // namespace openperf::network

#endif // _OP_CPU_GENERATOR_HPP_
