#include "worker.hpp"
#include "core/op_core.h"
#include "zmq.h"
#include "workers/random.hpp"


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

void block_worker::set_pattern(const model::block_generation_pattern& pattern)
{
    if (is_running()) {
        OP_LOG(OP_LOG_ERROR, "Cannot change pattern while worker is running");
        return;
    }
    clean_tasks();
    switch (pattern) {
    case model::block_generation_pattern::RANDOM:
        add_task<worker_task_random>(new worker::worker_task_random());
        break;
    case model::block_generation_pattern::SEQUENTIAL:
        add_task<worker_task_random>(new worker::worker_task_random());
        break;
    case model::block_generation_pattern::REVERSE:
        add_task<worker_task_random>(new worker::worker_task_random());
        break;
    }
}

void block_worker::update_state()
{
    zmq_send(m_socket.get(), std::addressof(state), sizeof(state), ZMQ_NOBLOCK);
}

void block_worker::submit_aio_op(const operation_config& op_config) {
    //auto buf = malloc(p_state.config.read_size);

    auto ai = ((struct aiocb) {
            .aio_fildes = op_config.fd,
            .aio_offset = op_config.offset,//p_state.config.read_size,
            .aio_buf = nullptr,
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
    worker_config config;
    worker_state* msg;
    msg = (worker_state*)malloc(sizeof(worker_state));
    for (;;)
    {
        int recv = zmq_recv(socket.get(), msg, sizeof(worker_state), running ? ZMQ_NOBLOCK : 0);
        running = msg->running;
        config = msg->config;
        fd = msg->fd;
        if (recv < 0 && errno != EAGAIN) {
            break;
        }
        if (running) {
            printf("%d\n", fd);
            submit_aio_op((worker_state){
                config,
                running,
                fd
            });
            for (auto& task : tasks) {
                task->read();
                task->write();
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    free(msg);
}

template<typename T>
void block_worker::add_task(T* task)
{
    tasks.push_back(worker_task_ptr(task));
};

void block_worker::clean_tasks()
{
    tasks.clear();
}

} // namespace openperf::block::worker