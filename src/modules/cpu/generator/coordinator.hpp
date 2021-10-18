#ifndef _OP_CPU_GENERATOR_COORDINATOR_HPP_
#define _OP_CPU_GENERATOR_COORDINATOR_HPP_

#include <optional>
#include <memory>
#include <thread>
#include <vector>

#include "cpu/generator/config.hpp"
#include "cpu/generator/worker_api.hpp"
#include "dynamic/spool.hpp"
#include "generator/v2/pid_controller.hpp"
#include "timesync/chrono.hpp"

namespace openperf::core {
class event_loop;
}

namespace openperf::cpu::generator {

class result
{
public:
    result(size_t size, const dynamic::configuration& dynamic_config);
    ~result() = default;

    using clock = openperf::timesync::chrono::realtime;
    using core_shard = core_stats<clock>;

    core_shard& shard(size_t idx);
    const std::vector<core_shard>& shards() const;

    dynamic::results dynamic() const;
    void do_dynamic_update();

private:
    std::vector<core_shard> m_shards;
    using dynamic_result = dynamic::spool<std::vector<core_shard>>;
    dynamic_result m_dynamic;
};

class coordinator
{
    void* m_context;
    worker::client m_client;
    config m_config;
    double m_setpoint;
    std::optional<uint32_t> m_loop_id;
    std::optional<result::core_shard> m_prev_sum;
    using pid_controller = openperf::generator::pid_controller<result::clock>;
    pid_controller m_pid;
    std::vector<std::thread> m_threads;
    std::shared_ptr<result> m_results;

public:
    coordinator(void* context, struct config&& conf);
    ~coordinator();

    const struct config& config() const;
    bool active() const;

    class generator::result* result() const;

    std::shared_ptr<generator::result>
    start(core::event_loop& loop, const dynamic::configuration& dynamic_config);
    void stop(core::event_loop& loop);

    int do_load_update();
};

} // namespace openperf::cpu::generator

#endif /* _OP_CPU_GENERATOR_COORDINATOR_HPP_ */
