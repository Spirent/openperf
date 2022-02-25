#ifndef _OP_MEMORY_GENERATOR_COORDINATOR_HPP_
#define _OP_MEMORY_GENERATOR_COORDINATOR_HPP_

#include "memory/generator/buffer.hpp"
#include "memory/generator/config.hpp"
#include "memory/generator/statistics.hpp"
#include "memory/generator/worker_api.hpp"
#include "dynamic/spool.hpp"
#include "generator/v2/pid_controller.hpp"
#include "timesync/chrono.hpp"

namespace openperf::core {
class event_loop;
}

namespace openperf::memory::generator {

class result
{
public:
    result(size_t size, const dynamic::configuration& dynamic_config);
    ~result() = default;

    using clock = openperf::timesync::chrono::realtime;
    using thread_shard = thread_stats<clock>;

    thread_shard& shard(size_t idx);
    const std::vector<thread_shard>& shards() const;

    dynamic::results dynamic() const;
    void do_dynamic_update();

private:
    std::vector<thread_shard> m_shards;
    using dynamic_result = dynamic::spool<std::vector<thread_shard>>;
    dynamic_result m_dynamic;
};

class coordinator
{
    using pid_controller = openperf::generator::pid_controller<result::clock>;

    struct io_pids
    {
        pid_controller read;
        pid_controller write;
    };

    void* m_context;
    worker::client m_client;
    config m_config;
    std::optional<uint32_t> m_loop_id;
    std::optional<result::thread_shard> m_prev_sum;
    io_pids m_pids;
    buffer m_buffer;
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

} // namespace openperf::memory::generator

#endif /* _OP_MEMORY_GENERATOR_COORDINATOR_HPP_ */
