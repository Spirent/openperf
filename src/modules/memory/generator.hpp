#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include <forward_list>
#include <future>
#include <atomic>

#include "memory/task_memory.hpp"
#include "memory/memory_stat.hpp"
#include "framework/generator/controller.hpp"

namespace openperf::memory::internal {

using namespace openperf::utils::worker;
using index_t = uint64_t;
using index_vector = std::vector<index_t>;

class generator
{
public:
    struct config_t
    {
        size_t buffer_size = 0;
        struct operation_config
        {
            size_t block_size = 0;
            size_t op_per_sec = 0;
            size_t threads = 0;
            io_pattern pattern = io_pattern::NONE;
        } read, write;
    };

    using time_point = std::chrono::system_clock::time_point;
    using controller = openperf::framework::generator::controller<memory_stat>;

private:
    bool m_stopped = true;
    bool m_paused = true;
    config_t m_config;

    std::thread m_scrub_thread;
    std::atomic_bool m_scrub_aborted;

    struct
    {
        void* ptr;
        size_t size;
    } m_buffer;

    index_vector m_read_indexes, m_write_indexes;
    std::future<index_vector> m_read_future, m_write_future;

    uint16_t m_serial_number;
    std::chrono::nanoseconds m_run_time;
    time_point m_run_time_milestone;
    std::atomic_int32_t m_init_percent_complete;

    controller m_controller;
    memory_stat m_stat;
    std::atomic<memory_stat*> m_stat_ptr;

public:
    // Constructors & Destructor
    generator();
    explicit generator(const generator::config_t&);
    generator(const generator&) = delete;
    generator(generator&&) noexcept;
    ~generator();

    // Operators overloading
    generator& operator=(generator&&) noexcept;
    generator& operator=(const generator&) = delete;

    // Methods
    void resume();
    void pause();

    void start();
    void stop();
    void restart();
    void reset();

    void config(const generator::config_t&);

    generator::config_t config() const { return m_config; }
    memory_stat stat() const;

    int32_t init_percent_complete() const
    {
        return m_init_percent_complete.load();
    }

    bool is_initialized() const { return init_percent_complete() == 100; }

    bool is_stopped() const { return m_stopped; }
    bool is_running() const { return !(m_paused || m_stopped); }
    bool is_paused() const { return m_paused; }

    static index_vector generate_index_vector(size_t size, io_pattern pattern);

private:
    void free_buffer();
    void resize_buffer(size_t);
    void scrub_worker();
    void spread_config(generator::workers&, const task_memory_config&);

    template <class T> void reallocate_workers(generator::workers&, unsigned);

    template <typename Function> void for_each_worker(Function&& function);
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_GENERATOR_HPP_
