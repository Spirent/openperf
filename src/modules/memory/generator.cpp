#include "memory/generator.hpp"
#include "memory/task_memory_read.hpp"
#include "memory/task_memory_write.hpp"
#include "utils/random.hpp"
#include "utils/memcpy.hpp"

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
    m_controller.process<memory_stat>([this](const memory_stat& stat) {
        auto elapsed_time = m_run_time;
        elapsed_time += std::chrono::system_clock::now() - m_run_time_milestone;

        auto stat_copy = m_stat;
        m_stat_ptr = &stat_copy;

        m_stat.read += stat.read;
        m_stat.write += stat.write;

        // Calculate really target values from start time of the generator
        if (m_config.read_threads) {
            auto& rstat = m_stat.read;
            rstat.operations_target =
                elapsed_time.count() * m_config.read.op_per_sec
                * std::min(m_config.read.block_size, 1UL) / std::nano::den;
            rstat.bytes_target =
                rstat.operations_target * m_config.read.block_size;
        }

        if (m_config.write_threads) {
            auto& wstat = m_stat.write;
            wstat.operations_target =
                elapsed_time.count() * m_config.write.op_per_sec
                * std::min(m_config.write.block_size, 1UL) / std::nano::den;
            wstat.bytes_target =
                wstat.operations_target * m_config.write.block_size;
        }

        m_stat_ptr = &m_stat;
    });
}

generator::generator(const generator::config_t& c)
    : generator()
{
    config(c);
}

generator::~generator()
{
    m_scrub_aborted.store(true);
    if (m_scrub_thread.joinable()) m_scrub_thread.join();
    if (m_read_future.valid()) m_read_future.wait();
    if (m_write_future.valid()) m_write_future.wait();

    stop();
    free_buffer();
}

// Methods : public
void generator::start()
{
    if (!m_stopped) return;
    if (m_read_future.valid()) {
        auto res = m_read_future.get();
        assert(res.size() == m_read_indexes.size());
        utils::memcpy(
            m_read_indexes.data(), res.data(), sizeof(index_t) * res.size());
    }
    if (m_write_future.valid()) {
        auto res = m_write_future.get();
        assert(res.size() == m_write_indexes.size());
        utils::memcpy(
            m_write_indexes.data(), res.data(), sizeof(index_t) * res.size());
    };

    reset();
    m_controller.resume();
    m_stopped = false;
    m_paused = false;
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

void generator::reset()
{
    m_controller.pause();
    m_controller.reset();
    m_stat = {};
    m_controller.resume();
}

memory_stat generator::stat() const
{
    auto stat = *m_stat_ptr;
    stat.active = is_running();

    return stat;
}

void generator::scrub_worker()
{
    size_t current = 0;
    auto block_size =
        ((m_buffer.size / 100 - 1) / getpagesize() + 1) * getpagesize();
    auto seed = utils::random_uniform<uint32_t>();
    while (!m_scrub_aborted.load() && current < m_buffer.size) {
        auto buf_len =
            std::min(static_cast<size_t>(block_size), m_buffer.size - current);
        utils::op_prbs23_fill(&seed, (uint8_t*)m_buffer.ptr + current, buf_len);
        current += buf_len;
        m_init_percent_complete.store(current * 100 / m_buffer.size);
    }
}

index_vector generator::generate_index_vector(size_t size, io_pattern pattern)
{
    index_vector indexes(size);
    std::iota(indexes.begin(), indexes.end(), 0);

    switch (pattern) {
    case io_pattern::SEQUENTIAL:
        break;
    case io_pattern::REVERSE:
        std::reverse(indexes.begin(), indexes.end());
        break;
    case io_pattern::RANDOM:
        // Use A Mersenne Twister pseudo-random generator to provide fast
        // vector shuffling
        {
            std::random_device rd;
            std::mt19937_64 g(rd());
            std::shuffle(indexes.begin(), indexes.end(), g);
        }
        break;
    default:
        OP_LOG(OP_LOG_ERROR, "Unrecognized generator pattern: %d\n", pattern);
    }

    return indexes;
}

/*
void generator::config(const generator::config_t& cfg)
{
    pause();
    resize_buffer(cfg.buffer_size);
    m_scrub_aborted.store(true);
    if (m_scrub_thread.joinable()) m_scrub_thread.join();
    m_scrub_aborted.store(false);

    m_init_percent_complete.store(0);
    m_scrub_thread = std::thread([this]() {
        scrub_worker();
        m_init_percent_complete.store(100);
        m_scrub_aborted.store(false);
    });

    auto configure_workers = [this](workers& w,
                                    std::vector<uint64_t>& indexes,
                                    const config_t::operation_config& op_cfg,
                                    std::future<index_vector>& future) {
        // io blocks in buffer
        size_t nb_blocks =
            op_cfg.block_size ? m_buffer.size / op_cfg.block_size : 0;

        if (future.valid()) future.wait();
        indexes.resize(nb_blocks);
        future = std::async(std::launch::async,
                            generate_index_vector,
                            nb_blocks,
                            op_cfg.pattern);

        auto rate = (!op_cfg.threads) ? 0 : op_cfg.op_per_sec / op_cfg.threads;

        spread_config(w,
                      task_memory_config{.block_size = op_cfg.block_size,
                                         .op_per_sec = rate,
                                         .pattern = op_cfg.pattern,
                                         .buffer = {.ptr = m_buffer.ptr,
                                                    .size = m_buffer.size},
                                         .indexes = &indexes});
    };

    reallocate_workers<task_memory_read>(m_read_workers, cfg.read.threads);
    configure_workers(m_read_workers, m_read_indexes, cfg.read, m_read_future);

    reallocate_workers<task_memory_write>(m_write_workers, cfg.write.threads);
    configure_workers(
        m_write_workers, m_write_indexes, cfg.write, m_write_future);
*/
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
     * because we expect this buffer to be relatively large, and OSv
     * currently can only allocate non-contiguous chunks of memory with
     * mmap. The buffer contents don't currently matter, so there is no
     * reason to initialize new buffers or shuffle contents between old and
     * new ones.
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
