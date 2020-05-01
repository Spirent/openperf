#include "memory/generator.hpp"
#include "memory/task_memory.hpp"

#include <cinttypes>
#include <sys/mman.h>

namespace openperf::memory::internal {

static uint16_t serial_counter = 0;

// Constructors & Destructor
generator::generator()
    : m_stopped(true)
    , m_paused(true)
    , m_buffer{.ptr = nullptr, .size = 0}
    , m_serial_number(++serial_counter)
{}

generator::generator(generator&& g)
    : m_stopped(g.m_stopped)
    , m_paused(g.m_paused)
    , m_read_workers(std::move(g.m_read_workers))
    , m_write_workers(std::move(g.m_write_workers))
    , m_config(g.m_config)
    , m_buffer{.ptr = nullptr, .size = 0}
    , m_serial_number(g.m_serial_number)
{}

generator::generator(const generator::config_t& c)
    : generator()
{
    config(c);
}

generator::~generator()
{
    stop();
    free_buffer();
}

// Methods : public
void generator::start()
{
    if (!m_stopped) return;

    for_each_worker([](worker_ptr& w) { w->start(); });
    m_stopped = false;
}

void generator::stop()
{
    if (m_stopped) return;

    for_each_worker([](worker_ptr& w) { w->stop(); });
    m_stopped = true;
}

void generator::restart()
{
    stop();
    start();
}

void generator::resume()
{
    if (!m_paused) return;

    for_each_worker([](worker_ptr& w) { w->resume(); });
    m_paused = false;
}

void generator::pause()
{
    if (m_paused) return;

    for_each_worker([](worker_ptr& w) { w->pause(); });
    m_paused = true;
}

void generator::reset()
{
    for_each_worker([](worker_ptr& w) {
        auto wtm = reinterpret_cast<worker<task_memory>*>(w.get());
        wtm->clear_stat();
    });
}

generator::stat_t generator::stat() const
{
    generator::stat_t result_stat;

    auto& rstat = result_stat.read;
    for (auto& ptr : m_read_workers) {
        auto w = reinterpret_cast<worker<task_memory>*>(ptr.get());
        rstat += w->stat();
    }

    auto& wstat = result_stat.write;
    for (auto& ptr : m_write_workers) {
        auto w = reinterpret_cast<worker<task_memory>*>(ptr.get());
        wstat += w->stat();
    }

    result_stat.timestamp = std::max(rstat.timestamp, wstat.timestamp);
    result_stat.active = is_running();
    return result_stat;
}

generator::config_t generator::config() const { return m_config; }

void generator::config(const generator::config_t& cfg)
{
    pause();
    resize_buffer(cfg.buffer_size);
    reallocate_workers<task_memory_read>(m_read_workers, cfg.read_threads);
    reallocate_workers<task_memory_write>(m_write_workers, cfg.write_threads);

    spread_config(m_read_workers,
                  task_memory_config{
                      .block_size = cfg.read.block_size,
                      .op_per_sec = cfg.read.op_per_sec,
                      .pattern = cfg.read.pattern,
                      .buffer = {.ptr = m_buffer.ptr, .size = m_buffer.size}});
    spread_config(m_write_workers,
                  task_memory_config{
                      .block_size = cfg.write.block_size,
                      .op_per_sec = cfg.write.op_per_sec,
                      .pattern = cfg.write.pattern,
                      .buffer = {.ptr = m_buffer.ptr, .size = m_buffer.size}});

    m_config = cfg;
    resume();
}

// Methods : private
void generator::for_each_worker(std::function<void(worker_ptr&)> function)
{
    for (auto list : {&m_read_workers, &m_write_workers}) {
        for (auto& worker : *list) { function(worker); }
    }
}

void generator::free_buffer()
{
    if (m_buffer.ptr == nullptr) return;
    if (m_buffer.ptr == MAP_FAILED) return;

    if (munmap(m_buffer.ptr, m_buffer.size) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to unallocate %zu"
               " bytes of memory: %s\n",
               m_buffer.size,
               strerror(errno));
    }

    m_buffer.ptr = MAP_FAILED;
    m_buffer.size = 0;
}

void generator::resize_buffer(size_t size)
{
    if (size == m_buffer.size) return;

    /*
     * We use mmap/munmap here instead of the standard allocation functions
     * because we expect this buffer to be relatively large, and OSv currently
     * can only allocate non-contiguous chunks of memory with mmap.
     * The buffer contents don't currently matter, so there is no reason to
     * initialize new buffers or shuffle contents between old and new ones.
     */
    free_buffer();
    if (size > 0) {
        OP_LOG(OP_LOG_INFO,
               "Reallocating buffer (%zu => %zu)\n",
               m_buffer.size,
               size);
        m_buffer.ptr = mmap(NULL,
                            size,
                            PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE,
                            -1,
                            0);
        if (m_buffer.ptr == MAP_FAILED) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed to allocate %" PRIu64 " byte memory buffer\n",
                   size);
            return;
        }
        m_buffer.size = size;
    }
}

void generator::spread_config(generator::workers& wkrs,
                              const task_memory_config& config)
{
    for (auto& w : wkrs) {
        auto wt = reinterpret_cast<worker<task_memory>*>(w.get());
        wt->config(config);
    }
}

} // namespace openperf::memory::internal
