#include "generator.hpp"
#include "task_memory_read.hpp"
#include "task_memory_write.hpp"

#include "framework/utils/random.hpp"
#include "framework/utils/memcpy.hpp"

#include <cinttypes>
#include <unistd.h>
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
    m_controller.start<memory_stat>([this](const memory_stat& stat) {
        auto elapsed_time = m_run_time;
        elapsed_time += std::chrono::system_clock::now() - m_run_time_milestone;

        auto stat_copy = m_stat;
        m_stat_ptr = &stat_copy;

        m_stat.read += stat.read;
        m_stat.write += stat.write;

        // Calculate really target values from start time of the generator
        if (m_config.read.threads) {
            auto& rstat = m_stat.read;
            rstat.operations_target =
                elapsed_time.count() * m_config.read.op_per_sec
                * std::min(m_config.read.block_size, 1UL) / std::nano::den;
            rstat.bytes_target =
                rstat.operations_target * m_config.read.block_size;
        }

        if (m_config.write.threads) {
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
    m_controller.clear();
    free_buffer();
}

// Methods : public
void generator::start()
{
    if (!m_stopped) return;

    if (m_read_future.valid()) {
        auto res = m_read_future.get();
        assert(res.size() == m_read_indexes.size());
        utils::memcpy(m_read_indexes.data(),
                      res.data(),
                      sizeof(index_vector::value_type) * res.size());
    }

    if (m_write_future.valid()) {
        auto res = m_write_future.get();
        assert(res.size() == m_write_indexes.size());
        utils::memcpy(m_write_indexes.data(),
                      res.data(),
                      sizeof(index_vector::value_type) * res.size());
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
    auto was_paused = m_paused;
    m_controller.pause();
    m_controller.reset();
    m_stat = {};

    if (!was_paused) m_controller.resume();
}

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

    m_scrub_aborted.store(true);
    if (m_scrub_thread.joinable()) m_scrub_thread.join();
    m_scrub_aborted.store(false);

    m_init_percent_complete.store(0);
    m_scrub_thread = std::thread([this]() {
        scrub_worker();
        m_init_percent_complete.store(100);
        m_scrub_aborted.store(false);
    });

    if (m_read_future.valid()) m_read_future.wait();
    m_read_indexes.resize(
        cfg.read.block_size ? m_buffer.size / cfg.read.block_size : 0);
    m_read_future = std::async(std::launch::async,
                               generate_index_vector,
                               m_read_indexes.size(),
                               cfg.read.pattern);

    for (size_t i = 0; i < cfg.read.threads; i++) {
        auto task = task_memory_read(task_memory_config{
            .block_size = cfg.read.block_size,
            .op_per_sec = cfg.read.op_per_sec / cfg.read.threads,
            .pattern = cfg.read.pattern,
            .buffer =
                {
                    .ptr = m_buffer.ptr,
                    .size = m_buffer.size,
                },
            .indexes = &m_write_indexes,
        });

        m_controller.add(std::move(task),
                         NAME_PREFIX + std::to_string(m_serial_number) + "_r"
                             + std::to_string(i + 1));
    }

    if (m_write_future.valid()) m_write_future.wait();
    m_write_indexes.resize(
        cfg.write.block_size ? m_buffer.size / cfg.write.block_size : 0);
    m_write_future = std::async(std::launch::async,
                                generate_index_vector,
                                m_write_indexes.size(),
                                cfg.write.pattern);

    for (size_t i = 0; i < cfg.write.threads; i++) {
        auto task = task_memory_write(task_memory_config{
            .block_size = cfg.write.block_size,
            .op_per_sec = cfg.write.op_per_sec / cfg.write.threads,
            .pattern = cfg.write.pattern,
            .buffer =
                {
                    .ptr = m_buffer.ptr,
                    .size = m_buffer.size,
                },
            .indexes = &m_write_indexes,
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

void generator::scrub_worker()
{
    size_t current = 0;
    auto block_size =
        ((m_buffer.size / 100 - 1) / getpagesize() + 1) * getpagesize();
    auto seed = utils::random_uniform<uint32_t>();
    while (!m_scrub_aborted.load() && current < m_buffer.size) {
        auto buf_len =
            std::min(static_cast<size_t>(block_size), m_buffer.size - current);
        utils::op_prbs23_fill(
            &seed, reinterpret_cast<uint8_t*>(m_buffer.ptr) + current, buf_len);
        current += buf_len;
        m_init_percent_complete.store(current * 100 / m_buffer.size);
    }
}

// Methods : static
generator::index_vector generator::generate_index_vector(size_t size,
                                                         io_pattern pattern)
{
    index_vector indexes(size);
    std::iota(indexes.begin(), indexes.end(), 0);

    switch (pattern) {
    case io_pattern::SEQUENTIAL:
        break;
    case io_pattern::REVERSE:
        std::reverse(indexes.begin(), indexes.end());
        break;
    case io_pattern::RANDOM: {
        // Use A Mersenne Twister pseudo-random generator to provide fast
        // vector shuffling
        std::random_device rd;
        std::mt19937_64 g(rd());
        std::shuffle(indexes.begin(), indexes.end(), g);
        break;
    }
    default:
        OP_LOG(OP_LOG_ERROR, "Unrecognized generator pattern: %d\n", pattern);
    }

    return indexes;
}

} // namespace openperf::memory::internal
