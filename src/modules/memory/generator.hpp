#ifndef _OP_MEMORY_GENERATOR_HPP_
#define _OP_MEMORY_GENERATOR_HPP_

#include <forward_list>

#include "utils/worker/worker.hpp"
#include "memory/task_memory_read.hpp"
#include "memory/task_memory_write.hpp"

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
        struct
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

private:
    bool m_stopped;
    bool m_paused;
    workers m_read_workers;
    workers m_write_workers;
    config_t m_config;

    struct
    {
        void* ptr;
        size_t size;
    } m_buffer;

public:
    // Constructors & Destructor
    generator();
    generator(generator&&);
    generator(const generator&) = delete;
    explicit generator(const generator::config_t&);
    ~generator();

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

    inline bool is_stopped() const { return m_stopped; }
    inline bool is_running() const { return !(m_paused || m_stopped); }
    inline bool is_paused() const { return m_paused; }

private:
    void free_buffer();
    void resize_buffer(size_t);
    void for_each_worker(std::function<void(worker_ptr&)>);
    void spread_config(generator::workers&, const task_memory_config&);

    template <class T> void reallocate_workers(generator::workers&, unsigned);
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
            wkrs.emplace_front(std::make_unique<worker<T>>("mem"));
        }
    }
}

} // namespace openperf::memory::internal

#endif // _OP_MEMORY_GENERATOR_HPP_
