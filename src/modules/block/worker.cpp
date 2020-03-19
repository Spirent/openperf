#include "worker.hpp"
#include "core/op_core.h"
#include "zmq.h"
#include "utils/random.hpp"
namespace openperf::block::worker 
{

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
    zmq_close(m_socket.get());
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
    zmq_send(m_socket.get(), std::addressof(state), sizeof(state), ZMQ_NOBLOCK);
}

off_t get_first_block(size_t, size_t)
{
    return 0;
}

off_t get_last_block(size_t file_size, size_t io_size)
{
    return static_cast<off_t>(file_size / io_size);
}

off_t pattern_random(pattern_state& p_state)
{
    p_state.idx = p_state.min + utils::random(p_state.max - p_state.min);
    return static_cast<off_t>(p_state.idx);
}

size_t pattern_sequential(pattern_state& p_state)
{
    if (++p_state.idx >= p_state.max) {
        p_state.idx = p_state.min;
    }

    return p_state.idx;
}

size_t pattern_reverse(pattern_state& p_state)
{
    if (--p_state.idx < p_state.min) {
        p_state.idx = p_state.max - 1;
    }

    return p_state.idx;
}

void submit_aio_op(const operation_config& op_config) {
    auto ai = ((struct aiocb) {
            .aio_fildes = op_config.fd,
            .aio_offset = static_cast<off_t>(op_config.block_size) * op_config.offset,
            .aio_buf = op_config.buffer,
            .aio_nbytes = op_config.block_size,
            .aio_reqprio = 0,
            .aio_sigevent.sigev_notify = SIGEV_NONE,
    });

    aio_write(&ai);
    struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };
    const struct aiocb *aiocblist[1] = {&ai};
    aio_suspend(aiocblist, 1, &timeout);
    printf("error: %d\n", aio_error(&ai));
    aio_return(&ai);
}

void block_worker::worker_loop(void* context)
{
    auto socket = std::unique_ptr<void, op_socket_deleter>(op_socket_get_client(context, ZMQ_PAIR, endpoint_prefix));
    bool running = false;
    int fd = 0;
    worker_pattern pattern;
    pattern_state pt_state = {0, 0, -1};
    worker_config config;
    auto msg = new worker_state();
    auto buf = (uint8_t*)malloc(1);

    for (;;)
    {
        int recv = zmq_recv(socket.get(), msg, sizeof(worker_state), running ? ZMQ_NOBLOCK : 0);
        if (recv < 0 && errno != EAGAIN) {
            break;
        }
        if (recv > 0) {
            running = msg->running;
            config = msg->config;
            pattern = msg->pattern;
            fd = msg->fd;
            pt_state = {0, get_last_block(config.f_size, config.read_size), -1};
        }

        printf("1- %ld\n",pt_state.max);
        printf("2- %ld\n",pt_state.idx);

        if (running) {
            printf("%d\n", fd);
            submit_aio_op((operation_config){
                fd,
                config.f_size,
                config.read_size,
                buf,
                pattern_random(pt_state),
                aio_write,
            });
            
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    free(msg);
    delete buf;
}

} // namespace openperf::block::worker