#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include <atomic>

#include "memory/task_memory.hpp"
#include "memory/memory_stat.hpp"
#include "framework/generator/controller.hpp"

namespace openperf::memory::internal {

class generator
{
public:
    struct config_t
    {
        size_t buffer_size = 0;
        size_t read_threads = 0;
        size_t write_threads = 0;
        struct
        {
            size_t block_size = 0;
            size_t op_per_sec = 0;
            io_pattern pattern = io_pattern::NONE;
        } read, write;
    };

    using time_point = std::chrono::system_clock::time_point;
    using controller = openperf::framework::generator::controller;

private:
    bool m_stopped = true;
    bool m_paused = true;
    config_t m_config;

    struct
    {
        void* ptr;
        size_t size;
    } m_buffer;

    uint16_t m_serial_number;
    std::chrono::nanoseconds m_run_time;
    time_point m_run_time_milestone;

    controller m_controller;
    memory_stat m_stat;
    std::atomic<memory_stat*> m_stat_ptr;

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
    void stop();
    void restart();
    void reset();

    void config(const generator::config_t&);

    generator::config_t config() const { return m_config; }
    memory_stat stat() const;

    bool is_stopped() const { return m_stopped; }
    bool is_running() const { return !(m_paused || m_stopped); }
    bool is_paused() const { return m_paused; }

private:
    void free_buffer();
    void resize_buffer(size_t);
};

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_GENERATOR_HPP_
