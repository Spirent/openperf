#include "memory/generator.hpp"
#include "memory/task_memory_read.hpp"
#include "memory/task_memory_write.hpp"

#include <cinttypes>
#include <sys/mman.h>

namespace openperf::memory::internal {

static uint16_t serial_counter = 0;
constexpr auto NAME_PREFIX = "op_mem";

// Constructors & Destructor
generator::generator()
    : m_buffer{.ptr = nullptr, .size = 0}
    , m_serial_number(++serial_counter)
    , m_controller(NAME_PREFIX + std::to_string(m_serial_number) + "_ctl")
    , m_stat_ptr(&m_stat)
{
    m_controller.processor([this](const memory_stat& stat) {
        auto elapsed_time = m_run_time;
        elapsed_time += std::chrono::system_clock::now() - m_run_time_milestone;

        auto stat_copy = m_stat;
        m_stat_ptr = &stat_copy;

        m_stat.read += stat.read;
        m_stat.write += stat.write;

        auto& rstat = m_stat.read;
        rstat.operations_target =
            elapsed_time.count() * m_config.read.op_per_sec
            * std::min(m_config.read.block_size, 1UL) / std::nano::den;
        rstat.bytes_target = rstat.operations_target * m_config.read.block_size;

        auto& wstat = m_stat.write;
        wstat.operations_target =
            elapsed_time.count() * m_config.write.op_per_sec
            * std::min(m_config.write.block_size, 1UL) / std::nano::den;
        wstat.bytes_target =
            wstat.operations_target * m_config.write.block_size;

        m_stat_ptr = &m_stat;
    });
}

generator::generator(generator&& g) noexcept
    : m_stopped(g.m_stopped)
    , m_paused(g.m_paused)
    , m_config(g.m_config)
    , m_buffer{.ptr = nullptr, .size = 0}
    , m_serial_number(g.m_serial_number)
    , m_controller(std::move(g.m_controller))
    , m_stat(std::move(g.m_stat))
    , m_stat_ptr(&m_stat)
{}

generator& generator::operator=(generator&& g) noexcept
{
    if (this != &g) {
        m_stopped = g.m_stopped;
        m_paused = g.m_paused;
        m_config = g.m_config;
        m_buffer = {.ptr = nullptr, .size = 0};
        m_serial_number = g.m_serial_number;
    }
    return (*this);
}

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

    m_controller.resume();
    m_stopped = false;
    m_run_time = 0ms;
    m_run_time_milestone = std::chrono::system_clock::now();
}

void generator::stop()
{
    if (m_stopped) return;

    m_controller.pause();
    m_stopped = true;
    m_run_time += std::chrono::system_clock::now() - m_run_time_milestone;
}

void generator::restart()
{
    stop();
    start();
}

void generator::resume()
{
    if (!m_paused) return;

    m_controller.resume();
    m_paused = false;
    m_run_time_milestone = std::chrono::system_clock::now();
}

void generator::pause()
{
    if (m_paused) return;

    m_controller.pause();
    m_paused = true;
    m_run_time += std::chrono::system_clock::now() - m_run_time_milestone;
}

void generator::reset() { m_controller.reset(); }

memory_stat generator::stat() const
{
    auto stat = *m_stat_ptr;
    stat.active = is_running();

    return stat;
}

void generator::config(const generator::config_t& cfg)
{
    auto was_paused = m_paused;
    pause();

    m_config = cfg;
    m_controller.clear();

    resize_buffer(cfg.buffer_size);

    for (size_t i = 0; i < cfg.read_threads; i++) {
        auto task = task_memory_read(task_memory_config{
            .block_size = cfg.read.block_size,
            .op_per_sec = cfg.read.op_per_sec / cfg.read_threads,
            .pattern = cfg.read.pattern,
            .buffer =
                {
                    .ptr = m_buffer.ptr,
                    .size = m_buffer.size,
                },
        });

        m_controller.add(std::move(task),
                         NAME_PREFIX + std::to_string(m_serial_number) + "_r"
                             + std::to_string(i + 1));
    }

    for (size_t i = 0; i < cfg.write_threads; i++) {
        auto task = task_memory_write(task_memory_config{
            .block_size = cfg.write.block_size,
            .op_per_sec = cfg.write.op_per_sec / cfg.write_threads,
            .pattern = cfg.write.pattern,
            .buffer =
                {
                    .ptr = m_buffer.ptr,
                    .size = m_buffer.size,
                },
        });

        m_controller.add(std::move(task),
                         NAME_PREFIX + std::to_string(m_serial_number) + "_w"
                             + std::to_string(i + 1));
    }

    if (!was_paused) resume();
}

// Methods : private
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

        m_buffer.ptr = mmap(nullptr,
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

} // namespace openperf::memory::internal
