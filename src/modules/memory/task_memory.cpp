#include "memory/task_memory.hpp"

#include <cstdint>

#include "core/op_core.h"
#include "utils/random.hpp"
#include "timesync/chrono.hpp"

namespace openperf::memory::internal {

using openperf::utils::random;
using openperf::utils::random_uniform;

uint64_t time_ns()
{
    return openperf::timesync::chrono::realtime::now()
        .time_since_epoch()
        .count();
};

const uint64_t NS_PER_SECOND = 1000000000ULL;

static const uint64_t QUANTA_MS = 10;
static const uint64_t MS_TO_NS = 1000000;
static const size_t MAX_SPIN_OPS = 5000;

void op_generator_sleep_until(uint64_t wake_time)
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

// Constructors & Destructor
task_memory::task_memory()
    : _config{ .pattern = io_pattern::NONE }
    , _buffer(nullptr)
    , _op_index_min(0)
    , _op_index_max(0)
    , _scratch_buffer(nullptr)
    , _scratch_size(0)
{
    scratch_allocate(4096);
    //_config.op_per_sec = 0;
    //[](uint64_t total, size_t buckets, size_t n) {
    //    assert(n < buckets);
    //    uint64_t base = total / buckets;
    //    return (n < total % buckets ? base + 1 : base);
    //} (10, 64, 8);
}

void task_memory::config(const task_memory_config& msg)
{
    assert(msg.pattern != io_pattern::NONE);
    // assert(msg->type == MEMORY_MSG_CONFIG);

    // io blocks in buffer
    size_t nb_blocks = (msg.block_size ? msg.buffer_size / msg.block_size : 0);

    ///* packed arrays are limited to 32 bi t unsigned integers */
    // if (nb_blocks > UINT_MAX) {
    //    OP_LOG(OP_LOG_ERROR, "Too many memory indexes!\n");
    //    return -1;
    //}

    /*
     * Need to update our indexes if the number of blocks or the pattern
     * has changed.
     */
    if (nb_blocks
        && (nb_blocks != _op_index_max || msg.pattern != _config.pattern)) {

        // if (_indexes) {
        //    op_packed_array_free(&_indexes);
        //}
        // assert(!_indexes);
        //_indexes = op_packed_array_allocate(nb_blocks, nb_blocks);
        // if (!_indexes) {
        //    OP_LOG(OP_LOG_ERROR, "Could not allocate %zu element index
        //    array\n",
        //            nb_blocks);
        //    _op_index_min = 0;
        //    _op_index_max = 0;
        //    return (-1);
        //}
        try {
            _indexes.resize(nb_blocks);
        } catch (std::exception e) {
            OP_LOG(OP_LOG_ERROR,
                   "Could not allocate %zu element index array\n",
                   nb_blocks);
            _op_index_min = 0;
            _op_index_max = 0;
            throw std::exception(e);
            //return;
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
            // op_packed_array_fill(_indexes, _op_index_min, 1);
            fill_vector(_op_index_min, 1);
            break;
        case io_pattern::REVERSE:
            // op_packed_array_fill(_indexes, _op_index_max - 1,
            // -1);
            fill_vector(_op_index_max - 1, -1);
            break;
        case io_pattern::RANDOM:
            // op_packed_array_fill(_indexes, _op_index_min, 1);
            fill_vector(_op_index_min, 1);
            // Shuffle array contents using the Fisher-Yates shuffle algorithm
            // op_packed_array_shuffle(_indexes);
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
    } else if (!nb_blocks) {
        // if (_indexes) {
        //    op_packed_array_free(&_indexes);
        //}
        _indexes.clear();
        _op_index_min = 0;
        _op_index_max = 0;
        _config.pattern = io_pattern::NONE;

        /* XXX: Can't generate any load without indexes */
        _config.op_per_sec = 0;
    }

    //_buffer = msg.buffer.ptr;
    _buffer = new uint8_t[msg.buffer_size];

    auto pseudo_random_fill = [](uint32_t* seedp, void* buffer, size_t length) {
        uint32_t seed = *seedp;
        uint32_t* ptr = reinterpret_cast<uint32_t*>(buffer);

        for (size_t i = 0; i < length / 4; i++) {
            uint32_t temp = (seed << 9) ^ (seed << 14);
            seed = temp ^ (temp >> 23) ^ (temp >> 18);
            *(ptr + i) = temp;
        }

        *seedp = seed;
    };

    if ((_config.block_size = msg.block_size) > _scratch_size) {
        OP_LOG(OP_LOG_DEBUG,
               "Reallocating scratch area (%zu --> %zu)\n",
               _scratch_size,
               msg.block_size);
        // icp_generator_free_scratch_area(&_scratch);
        scratch_free();
        //_scratch = icp_generator_allocate_scratch_area(msg.block_size);
        scratch_allocate(msg.block_size);
        assert(_scratch_buffer);
        uint32_t seed = random<uint32_t>();
        pseudo_random_fill(&seed, _scratch_buffer, _scratch_size);
    }

    _config = msg;

    // WTF callback?
    // if (msg.callback.fn && msg.callback.arg) {
    //    msg.callback.fn(msg.callback.arg, 1);
    //}
}

void task_memory::spin()
{
    /* If we have a rate to generate, then we need indexes */
    assert(_config.pattern != io_pattern::NONE);
    assert(_config.op_per_sec == 0 || _indexes.size() > 0);

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
    to_do_ops = std::max(0.0, c[0] * b[1] - b[0] * c[1]);
    ns_to_sleep = std::max(
        0.0, std::min(a[0] * c[1] - c[0] * a[1], (double)QUANTA_MS * MS_TO_NS));

    uint64_t t1 = time_ns();
    if (ns_to_sleep) {
        op_generator_sleep_until(t1 + ns_to_sleep);
        _total.sleep_time += time_ns() - t1;
    }
    /*
     * Perform load operations in small bursts so that we can update our
     * thread statistics periodically.
     */
    uint64_t deadline = time_ns() + (QUANTA_MS * MS_TO_NS);
    uint64_t t2;
    //std::cout << std::dec << "tdo: " << to_do_ops << ", deadline: " << deadline
    //          << ", to_sleep: " << ns_to_sleep << std::endl;
    while (to_do_ops && (t2 = time_ns()) < deadline) {
        size_t spin_ops = std::min(MAX_SPIN_OPS, to_do_ops);
        size_t nb_ops = operation(spin_ops, &op_index);
        uint64_t run_time =
            std::max(time_ns() - t2, 1lu); /* prevent divide by 0 */

        /* Update per thread statistics */
        _stats.time_ns += run_time;
        _stats.operations += nb_ops;
        _stats.bytes += nb_ops * _config.block_size;

        /* Update local counters */
        _total.run_time += run_time;
        _total.operations += nb_ops;
        _total.avg_rate +=
            ((double)(nb_ops / run_time) - _total.avg_rate + 4.0 / run_time)
            / 5;

        to_do_ops -= spin_ops;
    }

    static auto t = time_ns();
    if (time_ns() - t > 1000 * MS_TO_NS ) {
    std::cout << std::dec << "Total: { Ops: " << _total.operations
              << ", runtime: " << _total.run_time
              << ", avg: " << _total.avg_rate
              << ", sleep: " << _total.sleep_time
              << " } " << std::endl;

    std::cout << std::dec << "Stats: { time: " << _stats.time_ns
              << ", ops: " << _stats.operations
              << ", bytes: " << _stats.bytes
              << ", errors: " << _stats.errors
              << " } " << std::endl;
              t = time_ns();
    }
}

void task_memory::scratch_allocate(size_t size)
{
    // static uint64_t cachelinesize = 0;
    // if (!cachelinesize) {
    //    icp_config_get_value(sysinfo_cacheline_size, &cachelinesize);

    if (posix_memalign(&_scratch_buffer, _cache_size, size) != 0) {
        OP_LOG(OP_LOG_ERROR, "Could not allocate scratch area!\n");
        _scratch_buffer = nullptr;
    } else {
        _scratch_size = size;
    }
}

void task_memory::scratch_free()
{
    if (_scratch_buffer) {
        free(_scratch_buffer);
        _scratch_buffer = nullptr;
    }

    _scratch_size = 0;
}

} // namespace openperf::memory::internal
