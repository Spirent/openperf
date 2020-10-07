#include "generator.hpp"
#include "task_memory_read.hpp"
#include "task_memory_write.hpp"

#include "framework/utils/random.hpp"
#include "framework/utils/memcpy.hpp"

#include <cinttypes>
#include <string_view>
#include <unistd.h>
#include <sys/mman.h>

namespace openperf::memory::internal {

static uint16_t serial_counter = 0;

// Functions
generator::index_vector generate_index_vector(size_t size, io_pattern pattern)
{
    generator::index_vector indexes(size);
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

// Functions
std::optional<double> get_field(const memory_stat& stat, std::string_view name)
{
    if (name == "read.ops_target") return stat.read.operations_target;
    if (name == "read.ops_actual") return stat.read.operations;
    if (name == "read.bytes_target") return stat.read.bytes_target;
    if (name == "read.bytes_actual") return stat.read.bytes;
    if (name == "read.io_errors") return stat.read.errors;
    if (name == "read.latency_total") return stat.read.run_time.count();

    if (name == "read.latency_min")
        return stat.read.latency_min.value_or(0ns).count();
    if (name == "read.latency_max")
        return stat.read.latency_max.value_or(0ns).count();

    if (name == "write.ops_target") return stat.write.operations_target;
    if (name == "write.ops_actual") return stat.write.operations;
    if (name == "write.bytes_target") return stat.write.bytes_target;
    if (name == "write.bytes_actual") return stat.write.bytes;
    if (name == "write.io_errors") return stat.write.errors;
    if (name == "write.latency_total") return stat.write.run_time.count();

    if (name == "write.latency_min")
        return stat.write.latency_min.value_or(0ns).count();
    if (name == "write.latency_max")
        return stat.write.latency_max.value_or(0ns).count();

    if (name == "timestamp") return stat.timestamp().time_since_epoch().count();

    return std::nullopt;
}

// Constructors & Destructor
generator::generator()
    : m_buffer{.ptr = nullptr, .size = 0}
    , m_serial_number(++serial_counter)
    , m_stat_ptr(&m_stat)
    , m_dynamic(get_field)
    , m_controller(NAME_PREFIX + std::to_string(m_serial_number) + "_ctl")
{
    m_controller.start<memory_stat>([this](const memory_stat& stat) {
        auto elapsed_time = m_run_time;
        elapsed_time += std::chrono::system_clock::now() - m_run_time_milestone;

        auto stat_copy = m_stat;
        m_stat_ptr = &stat_copy;

        m_stat.read += stat.read;
        m_stat.write += stat.write;

        // Calculate really target values from start time of the generator
        if (m_read.config.threads) {
            auto& rstat = m_stat.read;
            rstat.operations_target =
                elapsed_time.count() * m_read.config.op_per_sec
                * std::min(m_read.config.block_size, 1UL) / std::nano::den;
            rstat.bytes_target =
                rstat.operations_target * m_read.config.block_size;
        }

        if (m_write.config.threads) {
            auto& wstat = m_stat.write;
            wstat.operations_target =
                elapsed_time.count() * m_write.config.op_per_sec
                * std::min(m_write.config.block_size, 1UL) / std::nano::den;
            wstat.bytes_target =
                wstat.operations_target * m_write.config.block_size;
        }

        m_dynamic.add(m_stat);
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
    if (m_read.future.valid()) m_read.future.wait();
    if (m_write.future.valid()) m_write.future.wait();

    stop();
    m_controller.clear();
    free_buffer();
}

// Methods : public
void generator::start()
{
    if (!m_stopped) return;

    if (m_read.future.valid()) {
        auto res = m_read.future.get();
        assert(res.size() == m_read.indexes.size());
        utils::memcpy(m_read.indexes.data(),
                      res.data(),
                      sizeof(index_vector::value_type) * res.size());
    }

    if (m_write.future.valid()) {
        auto res = m_write.future.get();
        assert(res.size() == m_write.indexes.size());
        utils::memcpy(m_write.indexes.data(),
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

void generator::start(const dynamic::configuration& cfg)
{
    if (!m_stopped) return;

    m_dynamic.configure(cfg);
    start();
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
    m_dynamic.reset();
    m_stat = {};
    m_stat.read.timestamp = m_stat.m_start_timestamp;
    m_stat.write.timestamp = m_stat.m_start_timestamp;

    if (!was_paused) m_controller.resume();
}

memory_stat generator::stat() const
{
    auto stat = *m_stat_ptr;
    stat.active = is_running();

    return stat;
}

dynamic::results generator::dynamic_results() const
{
    return m_dynamic.result();
}

void generator::config(const generator::config_t& cfg)
{
    auto was_paused = m_paused;
    pause();

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

    configure_tasks<task_memory_read>(m_read, cfg.read, "_r");
    configure_tasks<task_memory_write>(m_write, cfg.write, "_w");

    if (!was_paused) resume();
}

generator::config_t generator::config() const
{
    return {
        .buffer_size = m_buffer.size,
        .read = m_read.config,
        .write = m_write.config,
    };
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

void generator::init_index(operation_data& op)
{
    if (op.future.valid()) op.future.wait();

    // Resize index vector, because it will be passed to task configuration
    // and should has the valid size.
    op.indexes.resize(
        op.config.block_size ? m_buffer.size / op.config.block_size : 0);

    op.future = std::async(std::launch::async,
                           generate_index_vector,
                           op.indexes.size(),
                           op.config.pattern);
}

} // namespace openperf::memory::internal
