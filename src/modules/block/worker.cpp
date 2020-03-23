#include "worker.hpp"
#include "core/op_core.h"
#include "zmq.h"
#include "utils/random.hpp"
#include <time.h>
#include "timesync/chrono.hpp"
namespace openperf::block::worker
{

struct operation_config {
    int fd;
    size_t f_size;
    size_t block_size;
    uint8_t* buffer;
    off_t offset;
    int (*queue_aio_op)(aiocb *aiocb);
};

enum aio_state {
    IDLE = 0,
    PENDING,
    COMPLETE,
    FAILED,
};

struct operation_state {
    uint64_t start_ns;
    uint64_t stop_ns;
    uint64_t io_bytes;
    enum aio_state state;
    aiocb aiocb;
};

struct worker_data {
    worker_config config;
    bool running;
    int fd;
    pattern_generator pattern;
    operation_state *ops;
    uint8_t* buf;
};

struct worker_msg {
    worker_config config;
    bool running;
    int fd;
    worker_pattern pattern;
};

block_worker::block_worker(const worker_config& p_config, int p_fd, bool p_running, const model::block_generation_pattern& pattern):
    m_context(zmq_ctx_new()),
    m_socket(op_socket_get_server(m_context, ZMQ_PAIR, endpoint_prefix))
{
    auto p_thread = new std::thread([this]() {
        worker_loop(m_context);
    });
    worker_tread = std::unique_ptr<std::thread>(p_thread);

    state.config = p_config;
    state.running = p_running;
    state.fd = p_fd;

    set_pattern(pattern);
    update_state();
}

block_worker::~block_worker()
{
    zmq_close(m_socket);
    zmq_ctx_shutdown(m_context);
    zmq_ctx_term(m_context);
    worker_tread->detach();
}

bool block_worker::is_running() const
{
    return state.running;
}

void block_worker::set_resource_descriptor(int fd)
{
    state.fd = fd;
    update_state();
}

void block_worker::set_running(bool p_running)
{
    state.running = p_running;
    update_state();
}

void block_worker::set_pattern(const worker_pattern& pattern)
{
    if (is_running()) {
        OP_LOG(OP_LOG_ERROR, "Cannot change pattern while worker is running");
        return;
    }
    state.pattern = pattern;
    update_state();
}

void block_worker::update_state()
{
    zmq_send(m_socket, std::addressof(state), sizeof(state), ZMQ_NOBLOCK);
}

static off_t get_first_block(size_t, size_t)
{
    return 0;
}

static off_t get_last_block(size_t file_size, size_t io_size)
{
    return static_cast<off_t>(file_size / io_size);
}

static int submit_aio_op(const operation_config& op_config, operation_state *op_state) {
    struct aiocb *aio = &op_state->aiocb;
    *aio = ((aiocb) {
            .aio_fildes = op_config.fd,
            .aio_offset = static_cast<off_t>(op_config.block_size) * op_config.offset,
            .aio_buf = op_config.buffer,
            .aio_nbytes = op_config.block_size,
            .aio_reqprio = 0,
            .aio_sigevent.sigev_notify = SIGEV_NONE,
    });

    /* Reset stat related variables */
    op_state->state = PENDING;
    op_state->start_ns = 0;//icp_timestamp_now();
    op_state->stop_ns = 0;
    op_state->io_bytes = 0;

    /* Submit operation to the kernel */
    if (op_config.queue_aio_op(aio) == -1) {
        if (errno == EAGAIN) {
            OP_LOG(OP_LOG_WARNING, "AIO queue is full!\n");
            op_state->state = IDLE;
        } else {
            OP_LOG(OP_LOG_ERROR, "Could not queue AIO operation: %s\n",
                    strerror(errno));
        }
        return -errno;
    }
    return 0;
}

static int wait_for_aio_ops(operation_state *aio_ops,
                             size_t nb_ops) {

    const aiocb *aiocblist[nb_ops];
    size_t nb_cbs = 0;
    /*
     * Loop through all of our aio cb's and build a list with all pending
     * operations.
     */
    for (size_t i = 0; i < nb_ops; ++i) {
        if (aio_ops[i].state == PENDING) {
            /* add it to our list */
            aiocblist[nb_cbs++] = &aio_ops[i].aiocb;
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
        struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };
        if (aio_suspend(aiocblist, nb_cbs, &timeout) == -1) {
            error = -errno;
            OP_LOG(OP_LOG_ERROR, "aio_suspend failed: %s\n", strerror(errno));
        }
    }

    return (error);
}

static int complete_aio_op(struct operation_state *aio_op) {
int err = 0;
    struct aiocb *aiocb = &aio_op->aiocb;

    if (aio_op->state != PENDING)
        return (-1);

    switch((err = aio_error(aiocb))) {
    case 0: {
        /* AIO operation completed */
        ssize_t nb_bytes = aio_return(aiocb);
        aio_op->stop_ns = 0;//icp_timestamp_now();
        aio_op->io_bytes = nb_bytes;
        aio_op->state = COMPLETE;
        break;
    }
    case EINPROGRESS:
        /* Still waiting */
        break;
    case ECANCELED:
    default:
        /* could be canceled or error; we don't make a distinction */
        aio_return(aiocb);  /* free resources */
        aio_op->start_ns = 0;
        aio_op->io_bytes = 0;
        aio_op->state = FAILED;
        err = 0;
    }

    return (err);
}

static size_t worker_spin(worker_data& data, int (*queue_aio_op)(aiocb *aiocb))
{
    int32_t total_ops = 0;
    int32_t pending_ops = 0;
    auto op_conf = (operation_config){
        data.fd,
        data.config.f_size,
        data.config.read_size,
        data.buf,
        data.pattern.generate(),
        queue_aio_op
    };
    for (int32_t i = 0; i < data.config.queue_depth; ++i) {
        auto aio_op = &data.ops[i];
        if (submit_aio_op(op_conf, aio_op) == 0) {
            pending_ops++;
        } else if (aio_op->state == FAILED) {
            total_ops++;
            //atomic_inc(&stats->operations, 1);
            //atomic_inc(&stats->errors, 1);
        } else {
            /* temporary queueing error */
            break;
        }
    }
    while (pending_ops) {
        if (wait_for_aio_ops(data.ops, data.config.queue_depth) != 0) {
            /*
             * Eek!  Waiting failed, so cancel pending operations.
             * The aio_cancel function only has two documented failure modes:
             * 1) bad file descriptor
             * 2) aio_cancel not supported
             * We consider either of these conditions to be fatal.
             */
            if (aio_cancel(data.fd, NULL) == -1) {
                //icp_exit("Could not cancel pending AIO operatons: %s\n", strerror(errno));
            }
        }
        /*
         * Find the completed op and fire off another one to
         * take it's place.
         */
        for (int32_t i = 0; i < data.config.queue_depth; ++i) {
            auto aio_op = &data.ops[i];
            if (complete_aio_op(aio_op) == 0) {
                /* found it; update stats */
                total_ops++;
                switch (aio_op->state)
                {
                case COMPLETE:
                {
                    //atomic_inc(&stats->operations, 1);
                    //atomic_inc(&stats->bytes, aio_op->io_bytes);

                    /*uint64_t op_ns = aio_op->stop_ns - aio_op->start_ns;
                    atomic_inc(&stats->ns_total, op_ns);
                    if (op_ns < atomic_get(&stats->ns_min)) {
                        atomic_set(&stats->ns_min, op_ns);
                    }
                    if (op_ns > atomic_get(&stats->ns_max)) {
                        atomic_set(&stats->ns_max, op_ns);
                    }*/
                    break;
                }
                case FAILED:
                    //atomic_inc(&stats->operations, 1);
                    //atomic_inc(&stats->errors, 1);
                    break;
                default:
                    OP_LOG(OP_LOG_WARNING, "Completed AIO operation in %d state?\n",
                            aio_op->state);
                    break;
                }

                /* if we haven't hit our total or deadline, then fire off another op */
                if ((total_ops + pending_ops) >= data.config.queue_depth
                    //|| icp_timestamp_now() >= deadline
                    || submit_aio_op(op_conf, aio_op) != 0) {
                    // if any condition is true, we have one less pending op
                    pending_ops--;
                    if (aio_op->state == FAILED) {
                        total_ops++;
                        //atomic_inc(&stats->operations, 1);
                        //atomic_inc(&stats->errors, 1);
                    }
                }
            }
        }
    }
    return (total_ops);
}

void block_worker::worker_loop(void* context)
{
    auto socket = std::unique_ptr<void, op_socket_deleter>(op_socket_get_client(context, ZMQ_PAIR, endpoint_prefix));
    worker_data data;
    data.fd = -1;
    data.running = false;
    data.buf = 0;
    auto msg = new worker_msg();
    auto read_timestamp = timesync::chrono::realtime::now().time_since_epoch().count();
    auto write_timestamp = timesync::chrono::realtime::now().time_since_epoch().count();

    for (;;)
    {
        int recv = zmq_recv(socket.get(), msg, sizeof(worker_msg), data.running ? ZMQ_NOBLOCK : 0);
        if (recv < 0 && errno != EAGAIN) {
            break;
        }
        if (recv > 0) {
            data.running = msg->running;
            data.config = msg->config;
            data.fd = msg->fd;
            data.buf = (uint8_t*)realloc(data.buf, data.config.queue_depth * std::max(data.config.read_size, data.config.write_size));
            data.ops = (operation_state*)malloc(data.config.queue_depth * sizeof(operation_state));

            data.pattern.reset(get_first_block(data.config.f_size, data.config.read_size), get_last_block(data.config.f_size, data.config.read_size),msg->pattern);
            read_timestamp = timesync::chrono::realtime::now().time_since_epoch().count();
            write_timestamp = timesync::chrono::realtime::now().time_since_epoch().count();
        }
        if (data.running) {
            if (read_timestamp < write_timestamp) {
                worker_spin(data, aio_read);
                read_timestamp += std::nano::den / data.config.reads_per_sec;
            } else {
                worker_spin(data, aio_write);
                write_timestamp += std::nano::den / data.config.writes_per_sec;
            }
        }
        auto next_ts = std::min(read_timestamp, write_timestamp);
        auto now = timesync::chrono::realtime::now().time_since_epoch().count();
        if (next_ts > now)
            std::this_thread::sleep_for(std::chrono::nanoseconds(next_ts - now));
    }
    free(msg);
    if (data.buf)
        delete data.buf;
}

} // namespace openperf::block::worker