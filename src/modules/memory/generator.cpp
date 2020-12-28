#include "generator.hpp"
#include "info.hpp"
#include "task_memory_read.hpp"
#include "task_memory_write.hpp"

#include "framework/utils/random.hpp"
#include "framework/utils/memcpy.hpp"

#include <cinttypes>
#include <string_view>
#include <unistd.h>

namespace openperf::memory::internal {

static uint16_t serial_counter = 0;

// Functions
void buffer_initializer(std::weak_ptr<buffer> buffer_ptr,
                        std::atomic_int8_t& percent_complete)
{
    auto buffer = buffer_ptr.lock();
    percent_complete = 0;

    auto page_size = sysconf(_SC_PAGESIZE);
    auto block_size = ((buffer->size() / 100 - 1) / page_size + 1) * page_size;
    auto seed = utils::random_uniform<uint32_t>();

    for (size_t current = 0; current < buffer->size();) {
        auto buf_len =
            std::min(static_cast<size_t>(block_size), buffer->size() - current);
        utils::op_prbs23_fill(&seed,
                              reinterpret_cast<uint8_t*>(buffer->data())
                                  + current,
                              buf_len);
        current += buf_len;
        percent_complete = current * 100 / buffer->size();
    }

    percent_complete = 100;
}

index_vector generate_index_vector(size_t size, io_pattern pattern)
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
    : m_serial_number(++serial_counter)
    , m_stat_ptr(&m_stat)
    , m_dynamic(get_field)
    , m_controller(NAME_PREFIX + std::to_string(m_serial_number) + "_ctl")
{
    m_buffer = std::make_shared<buffer>();
    m_controller.start<memory_stat>([this](const memory_stat& stat) {
        auto elapsed_time = m_run_time;
        elapsed_time += std::chrono::system_clock::now() - m_run_time_milestone;

        auto stat_copy = m_stat;
        m_stat_ptr = &stat_copy;

        m_stat.read += stat.read;
        m_stat.write += stat.write;

        // Calculate really target values from start time of the generator
        using double_time = std::chrono::duration<double>;
        if (m_read.threads) {
            auto& rstat = m_stat.read;
            rstat.operations_target = static_cast<uint64_t>(
                (std::chrono::duration_cast<double_time>(elapsed_time)
                 * std::min(m_read.block_size, 1UL) * m_read.op_per_sec)
                    .count());
            rstat.bytes_target = rstat.operations_target * m_read.block_size;
        }

        if (m_write.threads) {
            auto& wstat = m_stat.write;
            wstat.operations_target = static_cast<uint64_t>(
                (std::chrono::duration_cast<double_time>(elapsed_time)
                 * std::min(m_write.block_size, 1UL) * m_write.op_per_sec)
                    .count());
            wstat.bytes_target = wstat.operations_target * m_write.block_size;
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
    stop();
    m_controller.clear();
}

// Methods : public
void generator::start()
{
    assert(0 < m_buffer->size());

    if (!m_stopped) return;
    if (m_buffer_initializer.valid()) { m_buffer_initializer.wait(); }

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
    assert(0 < cfg.buffer_size);

    auto was_paused = m_paused;
    pause();

    m_controller.clear();

    auto requested_size = cfg.buffer_size
                          + cfg.read.block_size * cfg.read.threads
                          + cfg.write.block_size * cfg.write.threads;

    if (auto free = memory_info::get().free; free < requested_size) {
        throw std::runtime_error(
            "Requested memory size is: " + std::to_string(requested_size)
            + " bytes (" + std::to_string(cfg.buffer_size) + " for buffer + "
            + std::to_string(cfg.read.threads) + " x "
            + std::to_string(cfg.read.block_size) + " for read blocks + "
            + std::to_string(cfg.write.threads) + " x "
            + std::to_string(cfg.write.block_size)
            + " for write blocks), but only " + std::to_string(free)
            + " bytes are available");
    }

    if (std::max(cfg.read.block_size, cfg.write.block_size) > cfg.buffer_size) {
        throw std::runtime_error(
            "Requested block size is greater than buffer (buffer: "
            + std::to_string(cfg.buffer_size)
            + ", read_block: " + std::to_string(cfg.read.block_size)
            + ", write_block: " + std::to_string(cfg.write.block_size));
    }

    m_buffer_size = cfg.buffer_size;
    if (auto buffer = cfg.buffer.lock()) {
        if (m_buffer_size > buffer->size()) {
            throw std::runtime_error(
                "Specified buffer size (" + std::to_string(m_buffer_size)
                + " bytes) more than actual size of shared buffer ("
                + std::to_string(buffer->size()) + " bytes)");
        }

        m_buffer = buffer;
        m_buffer_init_percent = 100;
    } else {
        m_buffer->resize(m_buffer_size);

        // Fill the buffer asynchronously
        if (m_buffer_initializer.valid()) { m_buffer_initializer.wait(); }
        m_buffer_initializer =
            std::async(std::launch::async,
                       buffer_initializer,
                       std::weak_ptr<internal::buffer>(m_buffer),
                       std::ref(m_buffer_init_percent));
    }

    m_read = cfg.read;
    m_write = cfg.write;

    configure_tasks<task_memory_read>(m_read, generate_index_vector, "_r");
    configure_tasks<task_memory_write>(m_write, generate_index_vector, "_w");

    if (!was_paused) resume();
}

generator::config_t generator::config() const
{
    return {
        .buffer_size = m_buffer_size,
        .read = m_read,
        .write = m_write,
    };
}

} // namespace openperf::memory::internal
