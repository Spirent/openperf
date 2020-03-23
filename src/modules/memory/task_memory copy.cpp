#include "memory/task_memory.hpp"

#include <cstdint>
#include <cassert>
#include <cstring>
#include <climits>
#include <chrono>

#include "core/op_core.h"
#include "timesync/chrono.hpp"
#include "memory/memory_generator.h"

#define atomic_inc(x, value)                                                   \
    atomic_fetch_add_explicit(x, value, memory_order_relaxed)

namespace openperf::memory::internal {

namespace timesync = openperf::timesync::chrono;

static const uint64_t QUANTA_MS = 10;
static const uint64_t MS_TO_NS = 1000000;
static const size_t MAX_SPIN_OPS = 5000;

const uint64_t NS_PER_SECOND = 1000000000ULL;
const uint64_t US_PER_SECOND = 1000000ULL;
const uint64_t MS_PER_SECOND = 1000ULL;

uint64_t icp_timestamp_now()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void icp_generator_sleep_until(uint64_t wake_time)
{
    uint64_t now = 0, ns_to_sleep = 0;
    struct timespec sleep = {0, 0};

    while ((now = icp_timestamp_now()) < wake_time) {
        ns_to_sleep = wake_time - now;

        if (ns_to_sleep < NS_PER_SECOND) {
            sleep.tv_nsec = ns_to_sleep;
        } else {
            sleep.tv_sec = ns_to_sleep / NS_PER_SECOND;
            sleep.tv_nsec = ns_to_sleep % NS_PER_SECOND;
        }

        nanosleep(&sleep, NULL);
    }
}

struct icp_generator_scratch_area
icp_generator_allocate_scratch_area(size_t size)
{
    static uint64_t cachelinesize = 0;
    if (!cachelinesize)
        icp_config_get_value("sysinfo.cachelinesize", &cachelinesize);

    struct icp_generator_scratch_area scratch = {
        .buffer = NULL,
        .size = 0,
    };

    if (posix_memalign(&scratch.buffer, cachelinesize, size) != 0) {
        OP_LOG(OP_LOG_ERROR, "Could not allocate scratch area!\n");
        scratch.buffer = NULL;
    } else {
        scratch.size = size;
    }

    return (scratch);
}

void icp_generator_free_scratch_area(struct icp_generator_scratch_area* scratch)
{
    assert(scratch);

    if (scratch->buffer) {
        free(scratch->buffer);
        scratch->buffer = NULL;
    }

    scratch->size = 0;
}

int _mem_worker_handle_abort(struct memory_generator_worker_data* data
                             __attribute__((unused)),
                             struct memory_generator_control_msg* msg
                             __attribute__((unused)))
{
    OP_LOG(OP_LOG_DEBUG, "Aborting!\n");
    op_task_sync_ping(data->context, msg->sync_endpoint);
    return (-1);
}

int _mem_worker_handle_stop(struct memory_generator_worker_data* data,
                            struct memory_generator_control_msg* msg)
{
    assert(msg->type == MEMORY_MSG_STOP);
    return (op_task_sync_ping(data->context, msg->sync_endpoint));
}

int _mem_worker_handle_invalid(struct memory_generator_worker_data* data,
                               struct memory_generator_control_msg* msg)
{
    OP_LOG(OP_LOG_ERROR,
           "Received invalid event %s while in state %s\n",
           _get_msg_type(msg->type),
           _get_worker_state(data->state));
    return (0);
}

int _mem_worker_handle_noop(struct memory_generator_worker_data* data
                            __attribute__((unused)),
                            struct memory_generator_control_msg* msg
                            __attribute__((unused)))
{
    return (0);
}

int _mem_worker_handle_config(struct memory_generator_worker_data* data,
                              struct memory_generator_control_msg* msg)
{
    assert(msg->type == MEMORY_MSG_CONFIG);

    size_t nb_blocks =
        (msg->config.io_size ? msg->config.buffer.size / msg->config.io_size
                             : 0);

    /* packed arrays are limited to 32 bit unsigned integers */
    if (nb_blocks > UINT_MAX) {
        OP_LOG(OP_LOG_ERROR, "Too many memory indexes!\n");
        return (-1);
    }

    /*
     * Need to update our indexes if the number of blocks or the pattern
     * has changed.
     */
    if (nb_blocks
        && (nb_blocks != data->config.op_index_max
            || msg->config.io_pattern != data->config.pattern)) {

        if (data->config.indexes) {
            op_packed_array_free(&data->config.indexes);
        }
        assert(!data->config.indexes);
        data->config.indexes = op_packed_array_allocate(nb_blocks, nb_blocks);
        if (!data->config.indexes) {
            OP_LOG(OP_LOG_ERROR,
                   "Could not allocate %zu element index array\n",
                   nb_blocks);
            data->config.op_index_min = 0;
            data->config.op_index_max = 0;
            return (-1);
        }
        data->config.op_index_min = 0;
        data->config.op_index_max = nb_blocks;

        /* Fill in the indexes... */
        switch (msg->config.io_pattern) {
        case GENERATOR_PATTERN_RANDOM:
            op_packed_array_fill(
                data->config.indexes, data->config.op_index_min, 1);
            op_packed_array_shuffle(data->config.indexes);
            break;
        case GENERATOR_PATTERN_SEQUENTIAL:
            op_packed_array_fill(
                data->config.indexes, data->config.op_index_min, 1);
            break;
        case GENERATOR_PATTERN_REVERSE:
            op_packed_array_fill(
                data->config.indexes, data->config.op_index_max - 1, -1);
            break;
        default:
            OP_LOG(OP_LOG_ERROR,
                   "Unrecognized generator pattern: %d\n",
                   msg->config.io_pattern);
        }
        data->config.pattern = msg->config.io_pattern;
    } else if (!nb_blocks) {
        if (data->config.indexes) {
            op_packed_array_free(&data->config.indexes);
        }
        data->config.op_index_min = 0;
        data->config.op_index_max = 0;
        data->config.pattern = GENERATOR_PATTERN_NONE;

        /* XXX: Can't generate any load without indexes */
        data->config.rate = 0;
    }

    data->config.buffer = msg->config.buffer.ptr;

    if ((data->config.op_block_size = msg->config.io_size)
        > data->scratch.size) {
        OP_LOG(OP_LOG_DEBUG,
               "Reallocating scratch area (%zu --> %zu)\n",
               data->scratch.size,
               msg->config.io_size);
        icp_generator_free_scratch_area(&data->scratch);
        data->scratch =
            icp_generator_allocate_scratch_area(msg->config.io_size);
        assert(data->scratch.buffer);
        uint32_t seed = icp_random();
        icp_pseudo_random_fill(&seed, data->scratch.buffer, data->scratch.size);
    }

    if (msg->config.callback.fn && msg->config.callback.arg) {
        msg->config.callback.fn(msg->config.callback.arg, 1);
    }

    return (0);
}

int _mem_worker_handle_load(struct memory_generator_worker_data* data,
                            struct memory_generator_control_msg* msg)
{
    assert(msg->type == MEMORY_MSG_LOAD);

    data->config.rate = icp_generator_distribute(
        msg->load.rate, data->worker.count, data->worker.idx);
    data->total.operations = 0;
    data->total.run_time = 0;
    data->total.sleep_time = 0;

    return (0);
}

int _mem_worker_handle_run(struct memory_generator_worker_data* data,
                           struct memory_generator_control_msg* msg
                           __attribute__((unused)))
{
    /* If we have a rate to generate, then we need indexes */
    assert(data->config.rate == 0 || data->config.indexes != NULL);

    static __thread size_t op_index = 0;
    if (op_index >= data->config.op_index_max) {
        op_index = data->config.op_index_min;
    }

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
    double ops_per_ns = (double)data->config.rate / NS_PER_SECOND;

    double a[2] = {1 - (ops_per_ns / data->total.avg_rate),
                   1.0 / data->total.avg_rate};
    double b[2] = {-ops_per_ns, 1};
    double c[2] = {ops_per_ns * (data->total.run_time + data->total.sleep_time)
                       - data->total.operations,
                   QUANTA_MS * MS_TO_NS};
    to_do_ops = op_max(0, c[0] * b[1] - b[0] * c[1]);
    ns_to_sleep =
        op_max(0, op_min(a[0] * c[1] - c[0] * a[1], QUANTA_MS * MS_TO_NS));

    uint64_t t1 = icp_timestamp_now();
    if (ns_to_sleep) {
        icp_generator_sleep_until(t1 + ns_to_sleep);
        data->total.sleep_time += icp_timestamp_now() - t1;
    }

    /*
     * Perform load operations in small bursts so that we can update our
     * thread statistics periodically.
     */
    uint64_t deadline = icp_timestamp_now() + (QUANTA_MS * MS_TO_NS);
    uint64_t t2;
    while (to_do_ops && (t2 = icp_timestamp_now()) < deadline) {
        size_t spin_ops = op_min(MAX_SPIN_OPS, to_do_ops);
        size_t nb_ops = data->memory_spin(
            &data->config, &data->scratch, spin_ops, &op_index);
        uint64_t run_time =
            op_max(icp_timestamp_now() - t2, 1); /* prevent divide by 0 */

        /* Update per thread statistics */
        atomic_inc(&data->stats->time_ns, run_time);
        atomic_inc(&data->stats->operations, nb_ops);
        atomic_inc(&data->stats->bytes, nb_ops * data->config.op_block_size);

        /* Update local counters */
        data->total.run_time += run_time;
        data->total.operations += nb_ops;
        data->total.avg_rate += ((double)(nb_ops / run_time)
                                 - data->total.avg_rate + 4.0 / run_time)
                                / 5;

        to_do_ops -= spin_ops;
    }

    return 0;
}

int _mem_worker_handle_running_config(struct memory_generator_worker_data* data,
                                      struct memory_generator_control_msg* msg)
{
    int error = _mem_worker_handle_config(data, msg);
    if (!error) { error = _mem_worker_handle_run(data, NULL); }

    return (error);
}

int _mem_worker_handle_running_load(struct memory_generator_worker_data* data,
                                    struct memory_generator_control_msg* msg)
{
    int error = _mem_worker_handle_load(data, msg);
    if (!error) { error = _mem_worker_handle_run(data, NULL); }

    return (error);
}

struct transition state_matrix[MEMORY_STATE_MAX][MEMORY_MSG_MAX] = {
    {{MEMORY_STATE_NONE, _mem_worker_handle_invalid},  /* None, None */
     {MEMORY_STATE_NONE, _mem_worker_handle_invalid},  /* None, Config */
     {MEMORY_STATE_NONE, _mem_worker_handle_invalid},  /* None, Load */
     {MEMORY_STATE_NONE, _mem_worker_handle_invalid},  /* None, Start */
     {MEMORY_STATE_NONE, _mem_worker_handle_invalid},  /* None, Stop */
     {MEMORY_STATE_NONE, _mem_worker_handle_invalid}}, /* None, Abort */
    {{MEMORY_STATE_IDLE, _mem_worker_handle_noop},     /* Idle, None */
     {MEMORY_STATE_READY, _mem_worker_handle_config},  /* Idle, Config */
     {MEMORY_STATE_IDLE, _mem_worker_handle_load},     /* Idle, Load */
     {MEMORY_STATE_NONE, _mem_worker_handle_invalid},  /* Idle, Start */
     {MEMORY_STATE_IDLE, _mem_worker_handle_stop},     /* Idle, Stop */
     {MEMORY_STATE_NONE, _mem_worker_handle_abort}},   /* Idle, Abort */
    {{MEMORY_STATE_READY, _mem_worker_handle_noop},    /* Ready, None */
     {
         MEMORY_STATE_READY,
         _mem_worker_handle_config,
     },                                              /* Ready, Config */
     {MEMORY_STATE_READY, _mem_worker_handle_load},  /* Ready, Load */
     {MEMORY_STATE_RUN, _mem_worker_handle_run},     /* Ready, Start */
     {MEMORY_STATE_READY, _mem_worker_handle_stop},  /* Ready, Stop */
     {MEMORY_STATE_NONE, _mem_worker_handle_abort}}, /* Ready, Abort */
    {{MEMORY_STATE_RUN, _mem_worker_handle_run},     /* Run, None */
     {
         MEMORY_STATE_RUN,
         _mem_worker_handle_running_config,
     },                                                   /* Run, Config */
     {MEMORY_STATE_RUN, _mem_worker_handle_running_load}, /* Run, Load */
     {MEMORY_STATE_RUN, _mem_worker_handle_run},          /* Run, Start */
     {MEMORY_STATE_READY, _mem_worker_handle_stop},       /* Run, Stop */
     {MEMORY_STATE_NONE, _mem_worker_handle_abort}}       /* Run, Abort */
};

int _mem_worker_get_control_msg(void* socket,
                                enum memory_generator_worker_state state,
                                struct memory_generator_control_msg* msg)
{
    int recv = zmq_recv(socket,
                        msg,
                        sizeof(*msg),
                        state == MEMORY_STATE_RUN ? ZMQ_DONTWAIT : 0);
    return (recv == -1 && errno == ETERM ? -1 : 0);
}

static size_t
_memory_read_spin(const struct memory_generator_worker_config* config,
                  struct icp_generator_scratch_area* scratch,
                  uint64_t nb_ops,
                  size_t* op_idx)
{
    assert(*op_idx < config->op_index_max);
    for (size_t i = 0; i < nb_ops; i++) {
        unsigned idx = op_packed_array_get(config->indexes, (*op_idx)++);
        std::memcpy(scratch->buffer,
                    config->buffer + idx * config->op_block_size,
                    config->op_block_size);
        if (*op_idx == config->op_index_max) { *op_idx = config->op_index_min; }
    }

    return nb_ops;
}

static size_t
_memory_write_spin(const struct memory_generator_worker_config* config,
                   struct icp_generator_scratch_area* scratch,
                   uint64_t nb_ops,
                   size_t* op_idx)
{
    assert(*op_idx < config->op_index_max);
    for (size_t i = 0; i < nb_ops; i++) {
        unsigned idx = op_packed_array_get(config->indexes, (*op_idx)++);
        std::memcpy(config->buffer + (idx * config->op_block_size),
                    scratch->buffer,
                    config->op_block_size);
        if (*op_idx == config->op_index_max) { *op_idx = config->op_index_min; }
    }

    return nb_ops;
}

void* memory_generator_worker_task(void* void_args)
{
    struct icp_task_args* targs = (struct icp_task_args*)void_args;

    const char* rw_tag = ((targs->custom.flags & 1) /* odd value means writer */
                              ? memory_generator_tag_write
                              : memory_generator_tag_read);
    /* clear last bit to get stats pointer */
    struct memory_generator_io_stats* stats = (void*)targs->custom.args;

    struct memory_generator_worker_data data = {
        .context = targs->context,
        .commands = op_socket_get_client_subscription(
            targs->context, memory_generator_commands, rw_tag),
        .scratch = icp_generator_allocate_scratch_area(4096),
        .stats = stats,
        .memory_spin = (strcmp(rw_tag, memory_generator_tag_read) == 0
                            ? _memory_read_spin
                            : _memory_write_spin),
        .total.avg_rate = 100000000,
        .state = MEMORY_STATE_IDLE,
        .worker = {
            .idx = (targs->custom.flags >> MEMORY_WORKER_IDX_SHIFT)
                   & MEMORY_WORKER_IDX_MASK,
            .count = (targs->custom.flags >> MEMORY_WORKER_COUNT_SHIFT)
                     & MEMORY_WORKER_COUNT_MASK,
        }};

    assert(data.commands);
    assert(data.scratch.buffer);
    free(targs);

    for (;;) {
        struct memory_generator_control_msg msg = {.type = MEMORY_MSG_NONE};

        if (_mem_worker_get_control_msg(data.commands, data.state, &msg) != 0) {
            break;
        }

        struct transition* t = &state_matrix[data.state][msg.type];
        data.state = t->next_state;
        int error = t->handler(&data, &msg);
        if (error) {
            OP_LOG(OP_LOG_ERROR,
                   "Transition to state %s due to event %s "
                   "failed: %s\n",
                   _get_worker_state(data.state),
                   _get_msg_type(msg.type),
                   strerror(error));
            data.state = MEMORY_STATE_NONE;
            break;
        }
    }

    // zmq_close(data.commands);
    if (data.config.indexes) { op_packed_array_free(&data.config.indexes); }
    icp_generator_free_scratch_area(&data.scratch);

    return (NULL);
}

void task_memory::run() {}

} // namespace openperf::memory::internal
