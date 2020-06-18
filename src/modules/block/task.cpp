#include <thread>
#include <limits>
#include "task.hpp"
#include "core/op_log.h"
#include "utils/random.hpp"

namespace openperf::block::worker {

using namespace std::chrono_literals;
constexpr duration TASK_SPIN_THRESHOLD = 100ms;

struct operation_config
{
    int fd;
    size_t f_size;
    size_t block_size;
    uint8_t* buffer;
    off_t offset;
    size_t header_size;
    int (*queue_aio_op)(aiocb* aiocb);
};

static int submit_aio_op(const operation_config& op_config,
                         operation_state& op_state)
{
    op_state.aiocb = ((aiocb){
        .aio_fildes = op_config.fd,
        .aio_offset = static_cast<off_t>(op_config.block_size * op_config.offset
                                         + op_config.header_size),
        .aio_buf = op_config.buffer,
        .aio_nbytes = op_config.block_size,
        .aio_reqprio = 0,
        .aio_sigevent.sigev_notify = SIGEV_NONE,
    });

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
        }
        return -errno;
    }
    return 0;
}

static int wait_for_aio_ops(std::vector<operation_state>& aio_ops,
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

    return (error);
}

static int complete_aio_op(struct operation_state& aio_op)
{
    int err = 0;
    auto aiocb = &aio_op.aiocb;

    if (aio_op.state != PENDING) return (-1);

    switch ((err = aio_error(aiocb))) {
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
    default:
        /* could be canceled or error; we don't make a distinction */
        aio_return(aiocb); /* free resources */
        aio_op.stop = ref_clock::now();
        aio_op.io_bytes = 0;
        aio_op.state = FAILED;
        err = 0;
    }

    return (err);
}

block_task::block_task()
    : m_actual_stat()
    , m_shared_stat(m_actual_stat)
    , m_at_stat(&m_shared_stat)
    , m_reset_stat(true)
    , m_operation_timestamp(ref_clock::now())
    , m_pause_timestamp(ref_clock::now())
    , m_start_timestamp(ref_clock::now())
{}

block_task::~block_task() = default;

size_t block_task::worker_spin(task_config_t& op_config,
                               task_stat_t& op_stat,
                               time_point deadline)
{
    size_t total_ops = 0;
    size_t pending_ops = 0;

    auto ops_req = (ref_clock::now() - m_operation_timestamp).count()
                       * op_config.ops_per_sec / std::nano::den
                   + 1;
    auto queue_depth =
        std::min(op_config.queue_depth, static_cast<size_t>(ops_req));

    auto op_conf = (operation_config){
        .fd = op_config.fd,
        .f_size = op_config.f_size,
        .block_size = op_config.block_size,
        .buffer = m_buf.data(),
        .offset = 0,
        .header_size = ((op_config.header_size - 1) / op_config.block_size + 1)
                       * op_config.block_size,
        .queue_aio_op = (op_config.operation == task_operation::READ)
                            ? aio_read
                            : aio_write};

    for (size_t i = 0; i < queue_depth; ++i) {
        auto& aio_op = m_aio_ops[i];
        op_conf.offset = m_pattern.generate();
        if (submit_aio_op(op_conf, aio_op) == 0) {
            pending_ops++;
        } else if (aio_op.state == FAILED) {
            total_ops++;
            op_stat.ops_actual++;
            op_stat.errors++;
        } else {
            /* temporary queueing error */
            break;
        }
        op_conf.buffer += op_config.block_size;
    }

    while (pending_ops) {
        if (wait_for_aio_ops(m_aio_ops, queue_depth) != 0) {
            /*
             * Eek!  Waiting failed, so cancel pending operations.
             * The aio_cancel function only has two documented failure modes:
             * 1) bad file descriptor
             * 2) aio_cancel not supported
             * We consider either of these conditions to be fatal.
             */
            if (aio_cancel(op_config.fd, nullptr) == -1) {
                OP_LOG(OP_LOG_ERROR,
                       "Could not cancel pending AIO operatons: %s\n",
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
                total_ops++;
                switch (aio_op.state) {
                case COMPLETE: {
                    op_stat.ops_actual++;
                    op_stat.bytes_actual += aio_op.io_bytes;

                    auto op_ns = aio_op.stop - aio_op.start;
                    op_stat.latency += op_ns;
                    if (op_ns < op_stat.latency_min) {
                        op_stat.latency_min = op_ns;
                    }
                    if (op_ns > op_stat.latency_max) {
                        op_stat.latency_max = op_ns;
                    }
                    break;
                }
                case FAILED:
                    op_stat.ops_actual++;
                    op_stat.errors++;
                    break;
                default:
                    OP_LOG(OP_LOG_WARNING,
                           "Completed AIO operation in %d state?\n",
                           aio_op.state);
                    break;
                }

                /* if we haven't hit our total or deadline, then fire off
                 * another op */
                if ((total_ops + pending_ops) >= queue_depth
                    || ref_clock::now() >= deadline
                    || submit_aio_op(op_conf, aio_op) != 0) {
                    // if any condition is true, we have one less pending op
                    pending_ops--;
                    if (aio_op.state == FAILED) {
                        total_ops++;
                        op_stat.ops_actual++;
                        op_stat.errors++;
                    }
                }
            }
        }
    }

    return (total_ops);
}

void block_task::config(const task_config_t& p_config)
{
    m_task_config = p_config;

    auto buf_len = m_task_config.queue_depth * m_task_config.block_size;
    m_buf.resize(buf_len);
    utils::op_pseudo_random_fill(m_buf.data(), m_buf.size());
    m_aio_ops.resize(m_task_config.queue_depth);
    m_pattern.reset(
        0,
        static_cast<off_t>((m_task_config.f_size - m_task_config.header_size)
                           / m_task_config.block_size),
        m_task_config.pattern);
}

task_config_t block_task::config() const { return m_task_config; }

void block_task::resume()
{
    m_operation_timestamp += ref_clock::now() - m_pause_timestamp;
    m_start_timestamp += ref_clock::now() - m_pause_timestamp;
}

void block_task::pause() { m_pause_timestamp = ref_clock::now(); }

task_stat_t block_task::stat() const
{
    auto res = *m_at_stat;
    res.latency_min = (res.latency_min == duration::max()) ? duration::zero()
                                                           : res.latency_min;
    return res;
}

void block_task::clear_stat()
{
    *m_at_stat = task_stat_t();
    m_reset_stat = true;
}

void block_task::reset_spin_stat()
{
    m_reset_stat = false;
    m_actual_stat = *m_at_stat;
    m_start_timestamp = ref_clock::now();
    m_operation_timestamp = m_start_timestamp;

    if (m_task_config.synchronizer) {
        if (m_task_config.operation == task_operation::READ)
            m_task_config.synchronizer->reads_actual.store(
                0, std::memory_order_relaxed);
        else
            m_task_config.synchronizer->writes_actual.store(
                0, std::memory_order_relaxed);
    }
}

void block_task::spin()
{
    if (m_reset_stat.load()) { reset_spin_stat(); }

    if (!m_task_config.ops_per_sec || !m_task_config.block_size)
        throw std::runtime_error(
            "Could not spin worker: no block operations were specified");

    // Reduce the effect of ZMQ operations on total Block I/O operations amount
    auto before_sleep_time = ref_clock::now();
    if (m_operation_timestamp > before_sleep_time) {
        std::this_thread::sleep_for(m_operation_timestamp - before_sleep_time);
    }

    auto check_synchronization = [&]() {
        if (!m_task_config.synchronizer) return false;

        if (m_task_config.operation == task_operation::READ)
            m_task_config.synchronizer->reads_actual.store(
                m_actual_stat.ops_actual, std::memory_order_relaxed);
        else
            m_task_config.synchronizer->writes_actual.store(
                m_actual_stat.ops_actual, std::memory_order_relaxed);

        int64_t reads_actual = m_task_config.synchronizer->reads_actual.load(
            std::memory_order_relaxed);
        int64_t writes_actual = m_task_config.synchronizer->writes_actual.load(
            std::memory_order_relaxed);

        int32_t ratio_reads = m_task_config.synchronizer->ratio_reads.load(
            std::memory_order_relaxed);
        int32_t ratio_writes = m_task_config.synchronizer->ratio_writes.load(
            std::memory_order_relaxed);

        switch (m_task_config.operation) {
        case task_operation::READ: {
            auto reads_expected = writes_actual * ratio_reads / ratio_writes;
            return reads_expected
                   < reads_actual
                         - std::max(
                             m_task_config.ops_per_sec * TASK_SPIN_THRESHOLD
                                 / 1s,
                             static_cast<int64_t>(m_task_config.queue_depth));
        }
        case task_operation::WRITE: {
            auto writes_expected = reads_actual * ratio_writes / ratio_reads;
            return writes_expected
                   < writes_actual
                         - std::max(
                             m_task_config.ops_per_sec * TASK_SPIN_THRESHOLD
                                 / 1s,
                             static_cast<int64_t>(m_task_config.queue_depth));
        }
        }
    };

    // Ratio synchronization
    if (check_synchronization()) {
        // sleep for single iteration
        std::this_thread::sleep_for(std::chrono::nanoseconds(
            std::nano::den / m_task_config.ops_per_sec));
        return;
    }

    // Worker loop
    auto loop_start_ts = ref_clock::now();
    do {
        auto cur_time = ref_clock::now();
        auto nb_ops = worker_spin(m_task_config, m_actual_stat, cur_time + 1s);

        auto cycles = (cur_time - m_start_timestamp).count()
                          * m_task_config.ops_per_sec / std::nano::den
                      + 1;
        m_actual_stat.ops_target = cycles;
        m_actual_stat.bytes_target = cycles * m_task_config.block_size;

        m_operation_timestamp +=
            std::chrono::nanoseconds(std::nano::den / m_task_config.ops_per_sec)
            * nb_ops;
    } while ((m_operation_timestamp < ref_clock::now())
             && (ref_clock::now() <= loop_start_ts + TASK_SPIN_THRESHOLD));
    m_actual_stat.updated = realtime::now();

    if (m_reset_stat.load()) { reset_spin_stat(); }

    *m_at_stat = m_actual_stat;
}

} // namespace openperf::block::worker