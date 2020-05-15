#include "memory/task_memory.hpp"

#include <algorithm>
#include <cstdint>
#include <chrono>
#include <tuple>
#include <thread>
#include <sys/mman.h>

#include "core/op_core.h"
#include "utils/random.hpp"
#include "timesync/chrono.hpp"

namespace openperf::memory::internal {

using namespace std::chrono_literals;

using openperf::utils::op_pseudo_random_fill;

auto now = openperf::timesync::chrono::monotime::now;

constexpr auto QUANTA = 10ms;
constexpr size_t MAX_SPIN_OPS = 5000;

auto calc_ops_and_sleep(const task_memory::total_t& total, size_t ops_per_sec)
{
    /*
     * Sleeping is problematic since you can't be sure if or when you'll
     * wake up.  Hence, we dynamically solve for the number of memory
     * operations to perform, to_do_ops, and for a requested time to sleep,
     * sleep time, using the following system of equations:
     *
     * 1. (m_total.operations + to_do_ops) /
     *      ((m_total.run_time + m_total.sleep_time)
     *      + (to_do_ops / m_total.avg_rate) + sleep_time = ops_per_ns
     * 2. to_do_ops / total.avg_rate + sleep_time = quanta
     *
     * We use Cramer's rule to solve for to_do_ops and sleep time.  We are
     * guaranteed a solution because our determinant is always 1.  However,
     * our solution could be negative, so clamp our answers before using.
     */
    double ops_per_ns = (double)ops_per_sec / std::nano::den;
    double a[2] = {1 - (ops_per_ns / total.avg_rate), 1.0 / total.avg_rate};
    double b[2] = {-ops_per_ns, 1};
    double c[2] = {ops_per_ns * (total.run_time + total.sleep_time).count()
                       - total.operations,
                   std::chrono::nanoseconds(QUANTA).count()};

    // Tuple: to_do_ops, ns_to_sleep
    return std::make_tuple(
        static_cast<uint64_t>(std::max(0.0, c[0] * b[1] - b[0] * c[1])),
        std::chrono::nanoseconds(
            static_cast<std::chrono::nanoseconds::rep>(std::max(
                0.0,
                std::min(a[0] * c[1] - c[0] * a[1],
                         (double)std::chrono::nanoseconds(QUANTA).count())))));
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

static uint64_t _get_cache_line_size()
{
    constexpr auto fname =
        "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size";

    uint64_t cachelinesize = 0;
    if (FILE* f = fopen(fname, "r"); !f) {
        OP_LOG(OP_LOG_WARNING,
               "Could not determine cache line size. "
               "Assuming 64 byte cache lines.\n");
        cachelinesize = 64;
    } else {
        fscanf(f, "%" PRIu64, &cachelinesize);
        fclose(f);
    }

    return cachelinesize;
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
    size_t nb_blocks = (msg.block_size ? msg.buffer.size / msg.block_size : 0);

    /*
     * Need to update our indexes if the number of blocks or the pattern
     * has changed.
     */
    if (nb_blocks
        && (nb_blocks != m_indexes.size() || msg.pattern != m_config.pattern)) {

        try {
            m_indexes = index_vector(nb_blocks, msg.pattern);
            m_config.pattern = msg.pattern;
        } catch (const std::exception& e) {
            OP_LOG(OP_LOG_ERROR,
                   "Could not allocate %zu element index array\n",
                   nb_blocks);
            throw std::exception(e);
        }

        // icp_generator_distribute
        // m_config.op_per_sec = [](uint64_t total, size_t buckets, size_t n) {
        //    assert(n < buckets);
        //    uint64_t base = total / buckets;
        //    return (n < total % buckets ? base + 1 : base);
        //} (10, 64, 8);
    } else if (!nb_blocks) {
        m_indexes.clear();
        m_config.pattern = io_pattern::NONE;

        /* XXX: Can't generate any load without indexes */
        m_config.op_per_sec = 0;
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
    assert(m_config.pattern != io_pattern::NONE);
    assert(m_config.op_per_sec == 0 || m_indexes.size() > 0);

    if (m_stat_clear) {
        m_stat_data = stat_t{};
        m_stat_clear = false;
    }

    static __thread size_t op_index = 0;
    if (op_index >= m_indexes.size()) { op_index = 0; }

    auto tuple = calc_ops_and_sleep(m_total, m_config.op_per_sec);
    auto to_do_ops = std::get<0>(tuple);

    if (auto ns_to_sleep = std::get<1>(tuple); ns_to_sleep.count()) {
        auto t1 = now();
        std::this_thread::sleep_for(ns_to_sleep);
        m_total.sleep_time += now() - t1;
    }
    /*
     * Perform load operations in small bursts so that we can update our
     * thread statistics periodically.
     */
    stat_t stat;
    auto deadline = now() + QUANTA;
    auto t2 = now();
    while (to_do_ops && (t2 = now()) < deadline) {
        size_t spin_ops = std::min(MAX_SPIN_OPS, to_do_ops);
        size_t nb_ops = operation(spin_ops, &op_index);
        auto run_time = std::max(now() - t2, 1ns); /* prevent divide by 0 */

        /* Update per thread statistics */
        stat += stat_t{
            .operations = nb_ops,
            .operations_target = spin_ops,
            .bytes = nb_ops * m_config.block_size,
            .bytes_target = spin_ops * m_config.block_size,
            .time = run_time,
            .latency_min = run_time,
            .latency_max = run_time,
        };

        /* Update local counters */
        m_total.run_time += run_time;
        m_total.operations += nb_ops;
        m_total.avg_rate += (nb_ops / (double)run_time.count()
                             - m_total.avg_rate + 4.0 / run_time.count())
                            / 5.0;

        to_do_ops -= spin_ops;
    }

    stat += m_stat_data;
    m_stat.store(&stat);
    m_stat_data = stat;
    m_stat.store(&m_stat_data);
}

void task_memory::scratch_allocate(size_t size)
{
    if (size == m_scratch.size) return;

    static uint64_t cache_line_size = 0;
    if (!cache_line_size) cache_line_size = _get_cache_line_size();

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
