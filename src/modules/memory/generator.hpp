#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include <atomic>
#include <future>

#include "buffer.hpp"
#include "memory_stat.hpp"
#include "task_memory.hpp"

#include "framework/generator/controller.hpp"
#include "modules/dynamic/spool.hpp"

namespace openperf::memory::internal {

class generator
{
    // Constants
    static constexpr auto NAME_PREFIX = "op_mem";

public:
    struct operation_config
    {
        size_t block_size = 0;
        size_t op_per_sec = 0;
        size_t threads = 0;
        io_pattern pattern = io_pattern::NONE;
    };

    struct config_t
    {
        size_t buffer_size = 0;
        operation_config read, write;
        std::weak_ptr<buffer> buffer;
    };

    using time_point = std::chrono::system_clock::time_point;
    using controller = openperf::framework::generator::controller;

private:
    bool m_stopped = true;
    bool m_paused = true;

    std::thread m_scrub_thread;
    std::atomic_bool m_scrub_aborted;

    size_t m_buffer_size = 0;
    std::shared_ptr<buffer> m_buffer;

    operation_config m_read, m_write;

    uint16_t m_serial_number;
    std::chrono::nanoseconds m_run_time;
    time_point m_run_time_milestone;
    std::atomic_int32_t m_init_percent_complete;

    memory_stat m_stat;
    std::atomic<memory_stat*> m_stat_ptr;

    dynamic::spool<memory_stat> m_dynamic;
    controller m_controller;

public:
    // Constructors & Destructor
    generator();
    explicit generator(const generator::config_t&);
    generator(const generator&) = delete;
    ~generator();

    // Operators overloading
    generator& operator=(const generator&) = delete;

    // Methods
    void resume();
    void pause();

    void start();
    void start(const dynamic::configuration&);
    void stop();
    void restart();
    void reset();

    void config(const generator::config_t&);

    generator::config_t config() const;
    memory_stat stat() const;
    dynamic::results dynamic_results() const;
    int32_t init_percent_complete() const { return m_init_percent_complete; }

    bool is_initialized() const { return init_percent_complete() == 100; }
    bool is_stopped() const { return m_stopped; }
    bool is_running() const { return !(m_paused || m_stopped); }
    bool is_paused() const { return m_paused; }

private:
    void scrub_worker();

    template <typename TaskType, typename Function>
    void configure_tasks(const operation_config&,
                         Function&& vector_generator,
                         const std::string& thread_suffix);
};

//
// Implementation
//

// Methods : private
template <typename TaskType, typename Function>
void generator::configure_tasks(const operation_config& op_config,
                                Function&& vector_generator,
                                const std::string& thread_suffix)
{
    size_t index_vector_size =
        op_config.block_size ? m_buffer_size / op_config.block_size : 0;

    auto index_initializer =
        std::shared_future<index_vector>(std::async(std::launch::async,
                                                    vector_generator,
                                                    index_vector_size,
                                                    op_config.pattern));

    for (size_t i = 0; i < op_config.threads; i++) {
        auto task = TaskType(task_memory_config{
            .block_size = op_config.block_size,
            .op_per_sec = op_config.op_per_sec / op_config.threads,
            .pattern = op_config.pattern,
            .buffer = m_buffer,
            .indexes = index_initializer,
        });

        m_controller.add(std::move(task),
                         NAME_PREFIX + std::to_string(m_serial_number)
                             + thread_suffix + std::to_string(i + 1));
    }
}

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_GENERATOR_HPP_
