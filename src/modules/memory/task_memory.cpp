#include <algorithm>
#include <cstdint>
#include <tuple>
#include <thread>
#include <sys/mman.h>

#include "framework/core/op_core.h"
#include "framework/utils/random.hpp"

#include "memory/task_memory.hpp"

namespace openperf::memory::internal {

constexpr auto QUANTA = 10ms;
constexpr size_t MAX_SPIN_OPS = 5000;

using namespace std::chrono_literals;
using openperf::utils::op_prbs23_fill;

auto calc_ops_and_sleep(const task_memory_stat& total,
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

// Constructors & Destructor
task_memory::task_memory(task_memory&& t) noexcept
    : m_config(std::move(t.m_config))
    , m_buffer(std::move(t.m_buffer))
    , m_scratch(std::move(t.m_scratch))
    , m_stat(t.m_stat)
    , m_op_index(t.m_op_index)
    , m_avg_rate(t.m_avg_rate)
    , m_rate(t.m_rate)
    , m_pid(t.m_pid)
{
    t.reset();
}

task_memory::task_memory(const task_memory_config& conf)
    : m_pid(0.01, 0.0005, 0.0)
{
    if (auto size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE); size > 0)
        m_scratch = buffer{static_cast<size_t>(size)};

    config(conf);
}

void task_memory::config(const task_memory_config& msg)
{
    assert(msg.pattern != io_pattern::NONE);

    m_op_index = utils::random_uniform(msg.indexes->size());
    m_config.pattern = msg.pattern;

    m_buffer = msg.buffer.lock();

    if ((m_config.block_size = msg.block_size) > m_scratch.size()) {
        OP_LOG(OP_LOG_DEBUG,
               "Reallocating scratch area (%zu --> %zu)\n",
               m_scratch.size(),
               msg.block_size);

        m_scratch.resize(msg.block_size);
        op_prbs23_fill(m_scratch.data(), m_scratch.size());
    }

    m_rate = msg.op_per_sec * std::min(msg.block_size, 1UL);
    m_pid.reset(m_rate);
    m_pid.max(m_rate * 2);

    m_config = msg;
}

void task_memory::reset()
{
    m_avg_rate = 100000000;
    m_stat = {};
    m_op_index = 0;

    m_rate = m_config.op_per_sec * std::min(m_config.block_size, 1UL);
    m_pid.reset(m_rate);
}

memory_stat task_memory::spin()
{
    /* If we have a rate to generate, then we need indexes */
    assert(m_rate == 0 || m_config.indexes->size() > 0);

    m_pid.start();

    auto tuple = calc_ops_and_sleep(m_stat, m_rate, m_avg_rate);
    auto to_do_ops = std::get<0>(tuple);

    task_memory_stat stat{};
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
        stat += task_memory_stat{
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

    using double_time = std::chrono::duration<double>;
    stat.operations_target =
        static_cast<uint64_t>((std::chrono::duration_cast<double_time>(
                                   stat.run_time + stat.sleep_time)
                               * m_rate)
                                  .count());
    stat.bytes_target = stat.operations_target * m_config.block_size;
    m_stat += stat;

    auto adjust = m_pid.stop(stat.operations);
    m_rate = static_cast<size_t>(
        m_config.op_per_sec * std::min(m_config.block_size, 1UL) + adjust);
    return make_stat(stat);
}

} // namespace openperf::memory::internal
