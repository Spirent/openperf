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

static void op_generator_sleep_until(uint64_t wake_time)
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
    : _op_index_min(0)
    , _op_index_max(0)
    , _buffer(nullptr)
    , _scratch{.ptr = nullptr, .size = 0}
    , _stat(&_stat_data)
    , _stat_clear(true)
{}

task_memory::task_memory(const task_memory_config& conf)
    : task_memory()
{
    config(conf);
}

task_memory::~task_memory() { scratch_free(); }

task_memory::stat_t task_memory::stat() const
{
    return (_stat_clear) ? stat_t{} : *_stat.load();
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
        && (nb_blocks != _op_index_max || msg.pattern != _config.pattern)) {

        try {
            _indexes.resize(nb_blocks);
        } catch (std::exception e) {
            OP_LOG(OP_LOG_ERROR,
                   "Could not allocate %zu element index array\n",
                   nb_blocks);
            _op_index_min = 0;
            _op_index_max = 0;
            throw std::exception(e);
        }

        _op_index_min = 0;
        _op_index_max = nb_blocks;

        auto fill_vector = [this](unsigned start, int step) {
            for (size_t i = 0; i < _indexes.size(); ++i) {
                _indexes[i] = start + (i * step);
            }
        };

        /* Fill in the indexes... */
        switch (msg.pattern) {
        case io_pattern::SEQUENTIAL:
            fill_vector(_op_index_min, 1);
            break;
        case io_pattern::REVERSE:
            fill_vector(_op_index_max - 1, -1);
            break;
        case io_pattern::RANDOM:
            fill_vector(_op_index_min, 1);
            // Shuffle array contents using the Fisher-Yates shuffle algorithm
            for (size_t i = _indexes.size() - 1; i > 0; --i) {
                auto j = random_uniform(i + 1);
                auto swap = _indexes[i];
                _indexes[i] = _indexes[j];
                _indexes[j] = swap;
            }
            break;
        default:
            OP_LOG(OP_LOG_ERROR,
                   "Unrecognized generator pattern: %d\n",
                   msg.pattern);
        }

        _config.pattern = msg.pattern;
        // icp_generator_distribute
        //_config.op_per_sec = [](uint64_t total, size_t buckets, size_t n) {
        //    assert(n < buckets);
        //    uint64_t base = total / buckets;
        //    return (n < total % buckets ? base + 1 : base);
        //} (10, 64, 8);
    } else if (!nb_blocks) {
        _indexes.clear();
        _op_index_min = 0;
        _op_index_max = 0;
        _config.pattern = io_pattern::NONE;

        /* XXX: Can't generate any load without indexes */
        _config.op_per_sec = 0;
    }

    _buffer = reinterpret_cast<uint8_t*>(msg.buffer.ptr);

    if ((_config.block_size = msg.block_size) > _scratch.size) {
        OP_LOG(OP_LOG_DEBUG,
               "Reallocating scratch area (%zu --> %zu)\n",
               _scratch.size,
               msg.block_size);
        scratch_allocate(msg.block_size);
        assert(_scratch.ptr);
        op_pseudo_random_fill(_scratch.ptr, _scratch.size);
    }

    _config = msg;
}

void task_memory::spin()
{
    /* If we have a rate to generate, then we need indexes */
    assert(_config.pattern != io_pattern::NONE);
    assert(_config.op_per_sec == 0 || _indexes.size() > 0);

    if (_stat_clear) {
        _stat_data = stat_t{};
        _stat_clear = false;
    }

    static __thread size_t op_index = 0;
    if (op_index >= _op_index_max) { op_index = _op_index_min; }

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
    double ops_per_ns = (double)_config.op_per_sec / NS_PER_SECOND;

    double a[2] = {1 - (ops_per_ns / _total.avg_rate), 1.0 / _total.avg_rate};
    double b[2] = {-ops_per_ns, 1};
    double c[2] = {ops_per_ns * (_total.run_time + _total.sleep_time)
                       - _total.operations,
                   QUANTA_MS * MS_TO_NS};
    to_do_ops = static_cast<uint64_t>(std::max(0.0, c[0] * b[1] - b[0] * c[1]));
    ns_to_sleep = static_cast<uint64_t>(std::max(
        0.0,
        std::min(a[0] * c[1] - c[0] * a[1], (double)QUANTA_MS * MS_TO_NS)));

    uint64_t t1 = time_ns();
    if (ns_to_sleep) {
        op_generator_sleep_until(t1 + ns_to_sleep);
        _total.sleep_time += time_ns() - t1;
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
            .bytes = nb_ops * _config.block_size,
            .bytes_target = spin_ops * _config.block_size,
            .time_ns = run_time,
            .latency_min = run_time,
            .latency_max = run_time,
        };

        /* Update local counters */
        _total.run_time += run_time;
        _total.operations += nb_ops;
        _total.avg_rate +=
            (nb_ops / (double)run_time - _total.avg_rate + 4.0 / run_time)
            / 5.0;

        to_do_ops -= spin_ops;
    }

    stat += _stat_data;
    _stat.store(&stat);
    _stat_data = stat;
    _stat.store(&_stat_data);
}

void task_memory::scratch_allocate(size_t size)
{
    if (size == _scratch.size) return;

    static uint64_t cache_line_size = 0;
    if (!cache_line_size) cache_line_size = _get_cache_line_size();

    scratch_free();
    if (size > 0) {
        if (posix_memalign(&_scratch.ptr, cache_line_size, size) != 0) {
            OP_LOG(OP_LOG_ERROR, "Could not allocate scratch area!\n");
            _scratch.ptr = nullptr;
        } else {
            _scratch.size = size;
        }
    }
}

void task_memory::scratch_free()
{
    if (_scratch.ptr != nullptr) {
        free(_scratch.ptr);
        _scratch.ptr = nullptr;
        _scratch.size = 0;
    }
}

} // namespace openperf::memory::internal
