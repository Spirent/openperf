#include "memory/task_memory.hpp"

#include <cstdint>
#include <cinttypes>
#include <sys/mman.h>

#include "core/op_core.h"
#include "utils/random.hpp"
#include "timesync/chrono.hpp"

namespace openperf::memory::internal {

using openperf::utils::op_pseudo_random_fill;
using openperf::utils::random_uniform;

const uint64_t NS_PER_SECOND = 1000000000ULL;

static const uint64_t QUANTA_MS = 10;
static const uint64_t MS_TO_NS = 1000000;
static const size_t MAX_SPIN_OPS = 5000;

static uint64_t time_ns()
{
    return openperf::timesync::chrono::realtime::now()
        .time_since_epoch()
        .count();
};

static void _generator_sleep_until(uint64_t wake_time)
{
    uint64_t now = 0, ns_to_sleep = 0;
    struct timespec sleep = {0, 0};

    while ((now = time_ns()) < wake_time) {
        ns_to_sleep = wake_time - now;

        if (ns_to_sleep < NS_PER_SECOND) {
            sleep.tv_nsec = ns_to_sleep;
        } else {
            sleep.tv_sec = ns_to_sleep / NS_PER_SECOND;
            sleep.tv_nsec = ns_to_sleep % NS_PER_SECOND;
        }

        nanosleep(&sleep, nullptr);
    }
}

static uint64_t _get_cache_line_size()
{
    FILE* f = NULL;
    uint64_t cachelinesize = 0;

    f = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size",
              "r");
    if (!f) {
        OP_LOG(OP_LOG_WARNING,
               "Could not determine cache line size. "
               "Assuming 64 byte cache lines.\n");
        cachelinesize = 64;
    } else {
        fscanf(f, "%" PRIu64, &cachelinesize);
        fclose(f);
    }

    return (cachelinesize);
}

// Constructors & Destructor
task_memory::task_memory()
    : m_op_index_min(0)
    , m_op_index_max(0)
    , m_buffer(nullptr)
    , m_scratch{.ptr = nullptr, .size = 0}
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
        && (nb_blocks != m_op_index_max || msg.pattern != m_config.pattern)) {

        try {
            m_indexes.resize(nb_blocks);
        } catch (std::exception e) {
            OP_LOG(OP_LOG_ERROR,
                   "Could not allocate %zu element index array\n",
                   nb_blocks);
            m_op_index_min = 0;
            m_op_index_max = 0;
            throw std::exception(e);
        }

        m_op_index_min = 0;
        m_op_index_max = nb_blocks;

        auto fill_vector = [this](unsigned start, int step) {
            for (size_t i = 0; i < m_indexes.size(); ++i) {
                m_indexes[i] = start + (i * step);
            }
        };

        /* Fill in the indexes... */
        switch (msg.pattern) {
        case io_pattern::SEQUENTIAL:
            fill_vector(m_op_index_min, 1);
            break;
        case io_pattern::REVERSE:
            fill_vector(m_op_index_max - 1, -1);
            break;
        case io_pattern::RANDOM:
            fill_vector(m_op_index_min, 1);
            // Shuffle array contents using the Fisher-Yates shuffle algorithm
            for (size_t i = m_indexes.size() - 1; i > 0; --i) {
                auto j = random_uniform(i + 1);
                auto swap = m_indexes[i];
                m_indexes[i] = m_indexes[j];
                m_indexes[j] = swap;
            }
            break;
        default:
            OP_LOG(OP_LOG_ERROR,
                   "Unrecognized generator pattern: %d\n",
                   msg.pattern);
        }

        m_config.pattern = msg.pattern;
        // icp_generator_distribute
        //m_config.op_per_sec = [](uint64_t total, size_t buckets, size_t n) {
        //    assert(n < buckets);
        //    uint64_t base = total / buckets;
        //    return (n < total % buckets ? base + 1 : base);
        //} (10, 64, 8);
    } else if (!nb_blocks) {
        m_indexes.clear();
        m_op_index_min = 0;
        m_op_index_max = 0;
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
    if (op_index >= m_op_index_max) { op_index = m_op_index_min; }

    /*
     * Sleeping is problematic since you can't be sure if or when you'll
     * wake up.  Hence, we dynamically solve for the number of memory
     * operations to perform, to_do_ops, and for a requested time to sleep,
     * sleep time, using the following system of equations:
     *
     * 1. (total.ops + to_do_ops)
     *     / ((total.run + total.sleep) + (to_do_ops / total.avg_rate) + sleep
     * time = rate
     * 2. to_do_ops / total.avg_rate + sleep_time = quanta
     *
     * We use Cramer's rule to solve for to_do_ops and sleep time.  We are
     * guaranteed a solution because our determinant is always 1.  However,
     * our solution could be negative, so clamp our answers before using.
     */
    uint64_t to_do_ops, ns_to_sleep;
    double ops_per_ns = (double)m_config.op_per_sec / NS_PER_SECOND;

    double a[2] = {1 - (ops_per_ns / m_total.avg_rate), 1.0 / m_total.avg_rate};
    double b[2] = {-ops_per_ns, 1};
    double c[2] = {ops_per_ns * (m_total.run_time + m_total.sleep_time)
                       - m_total.operations,
                   QUANTA_MS * MS_TO_NS};
    to_do_ops = static_cast<uint64_t>(std::max(0.0, c[0] * b[1] - b[0] * c[1]));
    ns_to_sleep = static_cast<uint64_t>(std::max(
        0.0,
        std::min(a[0] * c[1] - c[0] * a[1], (double)QUANTA_MS * MS_TO_NS)));

    uint64_t t1 = time_ns();
    if (ns_to_sleep) {
        _generator_sleep_until(t1 + ns_to_sleep);
        m_total.sleep_time += time_ns() - t1;
    }
    /*
     * Perform load operations in small bursts so that we can update our
     * thread statistics periodically.
     */
    stat_t stat;
    uint64_t deadline = time_ns() + (QUANTA_MS * MS_TO_NS);
    uint64_t t2;
    while (to_do_ops && (t2 = time_ns()) < deadline) {
        size_t spin_ops = std::min(MAX_SPIN_OPS, to_do_ops);
        size_t nb_ops = operation(spin_ops, &op_index);
        uint64_t run_time =
            std::max(time_ns() - t2, 1lu); /* prevent divide by 0 */

        /* Update per thread statistics */
        stat += stat_t{
            .operations = nb_ops,
            .operations_target = spin_ops,
            .bytes = nb_ops * m_config.block_size,
            .bytes_target = spin_ops * m_config.block_size,
            .time_ns = run_time,
            .latency_min = run_time,
            .latency_max = run_time,
        };

        /* Update local counters */
        m_total.run_time += run_time;
        m_total.operations += nb_ops;
        m_total.avg_rate +=
            (nb_ops / (double)run_time - m_total.avg_rate + 4.0 / run_time)
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
