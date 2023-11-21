#include <thread>
#include <limits>

#include "task.hpp"

#include "framework/core/op_log.h"
#include "framework/utils/random.hpp"

namespace openperf::block::worker {

using namespace std::chrono_literals;
constexpr duration TASK_SPIN_THRESHOLD = 100ms;

struct block_task::operation_config
{
    int fd;
    size_t f_size;
    size_t block_size;
    uint8_t* buffer;
    off_t offset;
    size_t header_size;
    int (*queue_aio_op)(aiocb* aiocb);
};

task_stat_t& task_stat_t::operator+=(const task_stat_t& stat)
{
    assert(operation == stat.operation);

    updated = std::max(updated, stat.updated);
    ops_target += stat.ops_target;
    ops_actual += stat.ops_actual;
    bytes_target += stat.bytes_target;
    bytes_actual += stat.bytes_actual;
    latency += stat.latency;

    latency_min = [&]() -> optional_time_t {
        if (latency_min.has_value() && stat.latency_min.has_value())
            return std::min(latency_min.value(), stat.latency_min.value());

        return latency_min.has_value() ? latency_min : stat.latency_min;
    }();

    latency_max = [&]() -> optional_time_t {
        if (latency_max.has_value() && stat.latency_max.has_value())
            return std::max(latency_max.value(), stat.latency_max.value());

        return latency_max.has_value() ? latency_max : stat.latency_max;
    }();

    return *this;
}

// Constructors & Destructor
block_task::block_task(const task_config_t& configuration)
{
    config(configuration);
}

// Methods : public
void block_task::reset()
{
    m_stat = {.operation = m_task_config.operation};
    m_operation_timestamp = {};

    if (m_task_config.synchronizer) {
        switch (m_task_config.operation) {
        case task_operation::READ:
            m_task_config.synchronizer->reads_actual.store(
                0, std::memory_order_relaxed);
            break;
        case task_operation::WRITE:
            m_task_config.synchronizer->writes_actual.store(
                0, std::memory_order_relaxed);
            break;
        }
    }
}

task_stat_t block_task::spin()
{
    if (!m_task_config.ops_per_sec || !m_task_config.block_size) {
        throw std::runtime_error(
            "Could not spin worker: no block operations were specified");
    }

    if (m_operation_timestamp.time_since_epoch() == 0ns) {
        m_operation_timestamp = ref_clock::now();
    }

    // Reduce the effect of ZMQ operations on total Block I/O operations amount
    auto sleep_time = m_operation_timestamp - ref_clock::now();
    if (sleep_time > 0ns) std::this_thread::sleep_for(sleep_time);

    // Prepare for ratio synchronization
    auto ops_per_sec = calculate_rate();

    // Worker loop
    auto stat = task_stat_t{.operation = m_task_config.operation};
    auto loop_start_ts = ref_clock::now();
    auto cur_time = ref_clock::now();
    do {
        // +1 need here to start spin at beginning of each time frame
        auto ops_req = (cur_time - m_operation_timestamp).count() * ops_per_sec
                           / std::nano::den
                       + 1;

        assert(ops_req);
        auto worker_spin_stat = worker_spin(ops_req, cur_time + 1s);
        stat += worker_spin_stat;

        cur_time = ref_clock::now();

        m_operation_timestamp +=
            std::chrono::nanoseconds(std::nano::den / ops_per_sec)
            * worker_spin_stat.ops_actual;

    } while ((m_operation_timestamp < cur_time) && !m_stopping
             && (cur_time <= loop_start_ts + TASK_SPIN_THRESHOLD));

    stat.updated = realtime::now();
    m_stat += stat;

    return stat;
}

// Methods : private
task_stat_t block_task::worker_spin(uint64_t nb_ops,
                                    ref_clock::time_point deadline)
{
    auto op_conf = operation_config{
        .fd = m_task_config.fd,
        .f_size = m_task_config.f_size,
        .block_size = m_task_config.block_size,
        .buffer = m_buf.data(),
        .offset = 0,
        .header_size =
            ((m_task_config.header_size - 1) / m_task_config.block_size + 1)
            * m_task_config.block_size,
        .queue_aio_op = (m_task_config.operation == task_operation::READ)
                            ? aio_read
                            : aio_write,
    };

    auto stat = task_stat_t{.operation = m_task_config.operation};
    size_t pending_ops = 0;
    auto queue_depth =
        std::min(m_task_config.queue_depth, static_cast<size_t>(nb_ops));
    bool cancelled = false;

    for (size_t i = 0; i < queue_depth; ++i) {
        auto& aio_op = m_aio_ops[i];
        op_conf.offset = m_pattern.generate();

        if (submit_aio_op(op_conf, aio_op) == 0) {
            pending_ops++;
        } else if (aio_op.state == FAILED) {
            stat.ops_actual++;
            stat.errors++;
        } else {
            /* temporary queueing error */
            break;
        }

        op_conf.buffer += m_task_config.block_size;
    }

    while (pending_ops) {
        if (m_stopping && !cancelled && ref_clock::now() >= deadline) {
            /*
             * The block generator is being stopped.
             * Cancel all pending requests and don't count them as errors.
             * This allow the block generator to stop faster.
             *
             * This was added because there were cases under heavy load
             * where block IO could take a long time to complete when
             * writing large block sizes.
             */
            if (aio_cancel(m_task_config.fd, nullptr) == -1) {
                OP_LOG(OP_LOG_ERROR,
                       "Could not cancel pending AIO operations: %s\n",
                       strerror(errno));
            } else {
                cancelled = true;
            }
        }

        if (wait_for_aio_ops(m_aio_ops, queue_depth) != 0) {
            /*
             * Eek!  Waiting failed, so cancel pending operations.
             * The aio_cancel function only has two documented failure modes:
             * 1) bad file descriptor
             * 2) aio_cancel not supported
             * We consider either of these conditions to be fatal.
             */
            if (aio_cancel(m_task_config.fd, nullptr) == -1) {
                OP_LOG(OP_LOG_ERROR,
                       "Could not cancel pending AIO operations: %s\n",
                       strerror(errno));
            }
        }

        /*
         * Find the completed op and fire off another one to
         * take it's place.
         */
        for (size_t i = 0; i < queue_depth; ++i) {
            auto& aio_op = m_aio_ops[i];
            if (complete_aio_op(aio_op) == 0) {
                /* found it; update stats */
                switch (aio_op.state) {
                case COMPLETE: {
                    stat.ops_actual++;
                    stat.bytes_actual += aio_op.io_bytes;

                    auto op_ns = aio_op.stop - aio_op.start;
                    stat.latency += op_ns;
                    stat.latency_min =
                        (stat.latency_min.has_value())
                            ? std::min(stat.latency_min.value(), op_ns)
                            : op_ns;
                    stat.latency_max =
                        (stat.latency_max.has_value())
                            ? std::max(stat.latency_max.value(), op_ns)
                            : op_ns;
                    break;
                }
                case CANCELLED:
                    if (!m_stopping) {
                        // Don't count cancelled operations as errors when
                        // stopping
                        stat.ops_actual++;
                        stat.errors++;
                    }
                    break;
                case FAILED:
                    stat.ops_actual++;
                    stat.errors++;
                    break;
                default:
                    OP_LOG(OP_LOG_WARNING,
                           "Completed AIO operation in %d state?\n",
                           aio_op.state);
                    break;
                }

                /* if we haven't hit our total or deadline, then fire off
                 * another op */
                // if ((total_ops + pending_ops) >= queue_depth
                if ((stat.ops_actual + pending_ops) >= queue_depth || m_stopping
                    || ref_clock::now() >= deadline
                    || submit_aio_op(op_conf, aio_op) != 0) {
                    // if any condition is true, we have one less pending op
                    pending_ops--;
                    if (aio_op.state == FAILED) {
                        stat.ops_actual++;
                        stat.errors++;
                    }
                }
            }
        }
    }

    return stat;
}

void block_task::config(const task_config_t& p_config)
{
    m_task_config = p_config;
    m_stat.operation = m_task_config.operation;

    auto buf_len = m_task_config.queue_depth * m_task_config.block_size;
    m_buf.resize(buf_len);
    utils::op_prbs23_fill(m_buf.data(), m_buf.size());
    m_aio_ops.resize(m_task_config.queue_depth);
    m_pattern.reset(
        0,
        static_cast<off_t>((m_task_config.f_size - m_task_config.header_size)
                           / m_task_config.block_size),
        m_task_config.pattern);
}

int32_t block_task::calculate_rate()
{
    if (!m_task_config.synchronizer) return m_task_config.ops_per_sec;

    if (m_task_config.operation == task_operation::READ)
        m_task_config.synchronizer->reads_actual.store(
            m_stat.ops_actual, std::memory_order_relaxed);
    else
        m_task_config.synchronizer->writes_actual.store(
            m_stat.ops_actual, std::memory_order_relaxed);

    int64_t reads_actual = m_task_config.synchronizer->reads_actual.load(
        std::memory_order_relaxed);
    int64_t writes_actual = m_task_config.synchronizer->writes_actual.load(
        std::memory_order_relaxed);

    int32_t ratio_reads =
        m_task_config.synchronizer->ratio_reads.load(std::memory_order_relaxed);
    int32_t ratio_writes = m_task_config.synchronizer->ratio_writes.load(
        std::memory_order_relaxed);

    switch (m_task_config.operation) {
    case task_operation::READ: {
        auto reads_expected = writes_actual * ratio_reads / ratio_writes;
        return std::min(
            std::max(reads_expected + m_task_config.ops_per_sec - reads_actual,
                     1L),
            static_cast<long>(m_task_config.ops_per_sec));
    }
    case task_operation::WRITE: {
        auto writes_expected = reads_actual * ratio_writes / ratio_reads;
        return std::min(std::max(writes_expected + m_task_config.ops_per_sec
                                     - writes_actual,
                                 1L),
                        static_cast<long>(m_task_config.ops_per_sec));
    }
    }
}

// Methods : private, static
int block_task::submit_aio_op(const operation_config& op_config,
                              operation_state& op_state)
{
    /* XXX: Proper initialization order depends on platform! */
    op_state.aiocb = aiocb{
        .aio_fildes = op_config.fd,
        .aio_reqprio = 0,
        .aio_buf = op_config.buffer,
        .aio_nbytes = op_config.block_size,
        .aio_sigevent.sigev_notify = SIGEV_NONE,
        .aio_offset = static_cast<off_t>(op_config.block_size * op_config.offset
                                         + op_config.header_size),
    };

    /* Reset stat related variables */
    op_state.state = PENDING;
    op_state.start = ref_clock::now();
    op_state.stop = op_state.start;
    op_state.io_bytes = 0;

    /* Submit operation to the kernel */
    if (op_config.queue_aio_op(&op_state.aiocb) == -1) {
        if (errno == EAGAIN) {
            OP_LOG(OP_LOG_WARNING, "AIO queue is full!\n");
            op_state.state = IDLE;
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "Could not queue AIO operation: %s\n",
                   strerror(errno));
            op_state.state = FAILED;
        }
        return -errno;
    }

    return 0;
}

int block_task::wait_for_aio_ops(std::vector<operation_state>& aio_ops,
                                 size_t nb_ops)
{
    const aiocb* aiocblist[nb_ops];
    size_t nb_cbs = 0;
    /*
     * Loop through all of our aio cb's and build a list with all pending
     * operations.
     */
    for (size_t i = 0; i < nb_ops; ++i) {
        if (aio_ops.at(i).state == PENDING) {
            aiocblist[nb_cbs++] = &aio_ops.at(i).aiocb;
        }
    }

    int error = 0;
    if (nb_cbs) {
        /*
         * Wait for one of our outstanding requests to finish.  The one
         * second timeout is a failsafe to prevent hanging here indefinitely.
         * It should (hopefully?) be high enough to prevent interrupting valid
         * block requests...
         */
        struct timespec timeout = {.tv_sec = 1, .tv_nsec = 0};
        if (aio_suspend(aiocblist, nb_cbs, &timeout) == -1) {
            error = -errno;
            OP_LOG(OP_LOG_ERROR, "aio_suspend failed: %s\n", strerror(errno));
        }
    }

    return error;
}

int block_task::complete_aio_op(struct operation_state& aio_op)
{
    int err = 0;
    auto aiocb = &aio_op.aiocb;

    if (aio_op.state != PENDING) return (-1);

    switch (err = aio_error(aiocb)) {
    case 0: {
        /* AIO operation completed */
        ssize_t nb_bytes = aio_return(aiocb);
        aio_op.stop = ref_clock::now();
        aio_op.io_bytes = nb_bytes;
        aio_op.state = COMPLETE;
        break;
    }
    case EINPROGRESS:
        /* Still waiting */
        break;
    case ECANCELED:
        /* AIO operation cancelled */
        aio_return(aiocb); /* free resources */
        aio_op.stop = ref_clock::now();
        aio_op.io_bytes = 0;
        aio_op.state = CANCELLED;
        err = 0;
        break;
    default:
        /* AIO operation failed */
        aio_return(aiocb); /* free resources */
        aio_op.stop = ref_clock::now();
        aio_op.io_bytes = 0;
        aio_op.state = FAILED;
        err = 0;
    }

    return err;
}

} // namespace openperf::block::worker
