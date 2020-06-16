#include "memory/task_memory.hpp"

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <thread>
#include <sys/mman.h>

#include "core/op_core.h"
#include "utils/random.hpp"

namespace openperf::memory::internal {

constexpr auto QUANTA = 10ms;
constexpr size_t MAX_SPIN_OPS = 5000;

using namespace std::chrono_literals;
using openperf::utils::op_pseudo_random_fill;

auto calc_ops_and_sleep(const task_memory::stat_t& total,
                        size_t ops_per_sec,
                        double avg_rate)
{
    /*
     * Sleeping is problematic since you can't be sure if or when you'll
     * wake up.  Hence, we dynamically solve for the number of memory
     * operations to perform, to_do_ops, and for a requested time to sleep,
     * sleep time, using the following system of equations:
     *
     * 1. (total.operations + to_do_ops) /
     *      ((total.run_time + total.sleep_time)
     *      + (to_do_ops / avg_rate) + sleep_time = ops_per_ns
     * 2. to_do_ops / avg_rate + sleep_time = quanta
     *
     * We use Cramer's rule to solve for to_do_ops and sleep time.  We are
     * guaranteed a solution because our determinant is always 1.  However,
     * our solution could be negative, so clamp our answers before using.
     */
    auto quanta_ns = static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(QUANTA).count());
    auto ops_per_ns = static_cast<double>(ops_per_sec) / std::nano::den;

    double a[2] = {1.0 - (ops_per_ns / avg_rate), 1.0 / avg_rate};
    double b[2] = {-ops_per_ns, 1.0};
    double c[2] = {ops_per_ns * (total.run_time + total.sleep_time).count()
                       - total.operations,
                   quanta_ns};

    // Tuple: to_do_ops, ns_to_sleep
    auto to_do_ops =
        static_cast<uint64_t>(std::max(0.0, c[0] * b[1] - b[0] * c[1]));
    auto ns_to_sleep =
        std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(
            std::max(0.0, std::min(a[0] * c[1] - c[0] * a[1], quanta_ns))));

    return std::make_tuple(to_do_ops, ns_to_sleep);
}

std::vector<unsigned> index_vector(size_t size, io_pattern pattern)
{
    std::vector<unsigned> v_index(size);
    std::iota(v_index.begin(), v_index.end(), 0);

    switch (pattern) {
    case io_pattern::SEQUENTIAL:
        break;
    case io_pattern::REVERSE:
        std::reverse(v_index.begin(), v_index.end());
        break;
    case io_pattern::RANDOM:
        std::shuffle(v_index.begin(), v_index.end(), std::random_device());
        break;
    default:
        OP_LOG(OP_LOG_ERROR, "Unrecognized generator pattern: %d\n", pattern);
    }

    return v_index;
}

// Constructors & Destructor
task_memory::task_memory()
    : m_scratch{.ptr = nullptr, .size = 0}
    , m_stat(&m_stat_data)
    , m_stat_clear(true)
{}

task_memory::task_memory(const task_memory_config& conf)
    : task_memory()
{
    config(conf);
}

task_memory::~task_memory() { scratch_free(); }

task_memory::stat_t task_memory::stat() const
{
    return (m_stat_clear) ? stat_t{} : *m_stat.load();
}

void task_memory::config(const task_memory_config& msg)
{
    assert(msg.pattern != io_pattern::NONE);

    // io blocks in buffer
    size_t nb_blocks = msg.block_size ? msg.buffer.size / msg.block_size : 0;

    /*
     * Need to update our indexes if the number of blocks or the pattern
     * has changed.
     */
    auto configuration_changed =
        nb_blocks != m_indexes.size() || msg.pattern != m_config.pattern;

    if (nb_blocks && configuration_changed) {
        try {
            m_op_index = 0;
            m_indexes = index_vector(nb_blocks, msg.pattern);
            m_config.pattern = msg.pattern;
        } catch (const std::exception& e) {
            OP_LOG(OP_LOG_ERROR,
                   "Could not allocate %zu element index array\n",
                   nb_blocks);
            throw std::exception(e);
        }
    } else if (!nb_blocks) {
        m_op_index = 0;
        m_indexes.clear();

        m_config.pattern = io_pattern::NONE;
        m_config.op_per_sec = 0;
        return;
    }

    m_buffer = reinterpret_cast<uint8_t*>(msg.buffer.ptr);

    if ((m_config.block_size = msg.block_size) > m_scratch.size) {
        OP_LOG(OP_LOG_DEBUG,
               "Reallocating scratch area (%zu --> %zu)\n",
               m_scratch.size,
               msg.block_size);
        scratch_allocate(msg.block_size);
        assert(m_scratch.ptr);
        op_pseudo_random_fill(m_scratch.ptr, m_scratch.size);
    }

    m_config = msg;
}

void task_memory::spin()
{
    /* If we have a rate to generate, then we need indexes */
    assert(m_config.op_per_sec == 0 || m_indexes.size() > 0);

    if (m_stat_clear) {
        m_avg_rate = 100000000;
        m_stat_data = stat_t{};
        m_stat_clear = false;
    }

    auto tuple =
        calc_ops_and_sleep(m_stat_data, m_config.op_per_sec, m_avg_rate);
    auto to_do_ops = std::get<0>(tuple);

    stat_t stat{};
    if (auto ns_to_sleep = std::get<1>(tuple); ns_to_sleep.count()) {
        auto start = chronometer::now();
        std::this_thread::sleep_for(ns_to_sleep);
        stat.sleep_time = chronometer::now() - start;
    }

    /*
     * Perform load operations in small bursts so that we can update our
     * thread statistics periodically.
     */
    auto ts = chronometer::now();
    auto deadline = ts + QUANTA;
    while (to_do_ops && (ts = chronometer::now()) < deadline) {
        size_t spin_ops = std::min(MAX_SPIN_OPS, to_do_ops);
        operation(spin_ops);
        auto run_time =
            std::max(chronometer::now() - ts, 1ns); /* prevent divide by 0 */

        /* Update per thread statistics */
        stat += stat_t{
            .operations = spin_ops,
            .bytes = spin_ops * m_config.block_size,
            .run_time = run_time,
            .latency_min = run_time,
            .latency_max = run_time,
        };

        /* Update local counters */
        m_avg_rate += (static_cast<double>(spin_ops) / run_time.count()
                       - m_avg_rate + 4.0 / run_time.count())
                      / 5.0;

        to_do_ops -= spin_ops;
    }

    stat += m_stat_data;
    stat.operations_target = (stat.run_time + stat.sleep_time).count()
                             * m_config.op_per_sec / std::nano::den;
    stat.bytes_target = stat.operations_target * m_config.block_size;

    m_stat.store(&stat);
    m_stat_data = stat;
    m_stat.store(&m_stat_data);
}

void task_memory::scratch_allocate(size_t size)
{
    if (size == m_scratch.size) return;

    static uint16_t cache_line_size = 0;
    if (!cache_line_size) cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);

    scratch_free();
    if (size > 0) {
        if (posix_memalign(&m_scratch.ptr, cache_line_size, size) != 0) {
            OP_LOG(OP_LOG_ERROR, "Could not allocate scratch area!\n");
            m_scratch.ptr = nullptr;
        } else {
            m_scratch.size = size;
        }
    }
}

void task_memory::scratch_free()
{
    if (m_scratch.ptr != nullptr) {
        free(m_scratch.ptr);
        m_scratch.ptr = nullptr;
        m_scratch.size = 0;
    }
}

} // namespace openperf::memory::internal
