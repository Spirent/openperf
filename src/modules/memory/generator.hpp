#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include <forward_list>
#include <future>

#include "utils/worker/worker.hpp"
#include "memory/task_memory.hpp"
#include "memory/memory_stat.hpp"

namespace openperf::memory::internal {

using namespace openperf::utils::worker;

class generator
{
public:
    struct config_t
    {
        size_t buffer_size = 0;
        size_t read_threads = 0;
        size_t write_threads = 0;
        struct operation_config
        {
            size_t block_size = 0;
            size_t op_per_sec = 0;
            io_pattern pattern = io_pattern::NONE;
        } read, write;
    };

    struct stat_t
    {
        bool active;
        memory_stat::timestamp_t timestamp;
        memory_stat read;
        memory_stat write;
    };

private:
    using worker_ptr = std::unique_ptr<workable>;
    using workers = std::forward_list<worker_ptr>;
    using time_point = std::chrono::system_clock::time_point;

private:
    bool m_stopped = true;
    bool m_paused = true;
    workers m_read_workers;
    workers m_write_workers;
    config_t m_config;

    std::thread m_scrub_thread;
    std::atomic_bool m_scrub_aborted;

    struct
    {
        void* ptr;
        size_t size;
    } m_buffer;

    std::vector<unsigned> m_read_indexes, m_write_indexes;
    std::future<void> m_read_future, m_write_future;

    uint16_t m_serial_number;
    std::chrono::nanoseconds m_run_time;
    time_point m_run_time_milestone;
    std::atomic_int32_t m_init_percent_complete;

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

    generator::config_t config() const;
    generator::stat_t stat() const;

    int32_t init_percent_complete() const
    {
        return m_init_percent_complete.load();
    }

    bool is_initialized() const { return init_percent_complete() == 100; }

    bool is_stopped() const { return m_stopped; }
    bool is_running() const { return !(m_paused || m_stopped); }
    bool is_paused() const { return m_paused; }

private:
    void free_buffer();
    void resize_buffer(size_t);
    void scrub_worker();
    void spread_config(generator::workers&, const task_memory_config&);

    template <class T> void reallocate_workers(generator::workers&, unsigned);

    template <typename Function> void for_each_worker(Function&& function);
};

template <class T>
void generator::reallocate_workers(generator::workers& wkrs, unsigned num)
{
    assert(num >= 0);
    if (num == 0) {
        wkrs.clear();
        return;
    }

    auto size = std::distance(wkrs.begin(), wkrs.end());
    if (num < size) {
        for (; size > num; --size) { wkrs.pop_front(); }
    } else {
        for (; size < num; ++size) {
            wkrs.emplace_front(std::make_unique<worker<T>>(
                task_memory_config{},
                "mem" + std::to_string(m_serial_number) + "_"
                    + std::to_string(num - size)));
        }
    }
}

template <typename Function>
void generator::for_each_worker(Function&& function)
{
    std::for_each(m_read_workers.begin(), m_read_workers.end(), function);
    std::for_each(m_write_workers.begin(), m_write_workers.end(), function);
}

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_GENERATOR_HPP_
