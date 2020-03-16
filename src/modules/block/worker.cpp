#include "worker.hpp"
#include "core/op_core.h"
#include "zmq.h"
#include "block/workers/random.hpp"


namespace openperf::block::worker 
{

block_worker::block_worker(const worker_config& p_config, bool p_running, const std::string& pattern):
    m_context(zmq_ctx_new()),
    m_socket(op_socket_get_server(m_context, ZMQ_PAIR, endpoint_prefix))
{
    auto p_thread = new std::thread([this]() {
        worker_loop(m_context);
    });
    worker_tread = std::unique_ptr<std::thread>(p_thread);

    state.config = p_config;
    state.running = p_running;

    if (pattern == "random") {
        add_task<worker_task_random>(new worker::worker_task_random());
    }
}

block_worker::~block_worker() 
{
    zmq_ctx_shutdown(m_context);
    worker_tread->detach();
    tasks.clear();
}

void block_worker::set_running(bool p_running) 
{
    state.running = p_running;
    update_state();
}

void block_worker::set_pattern(const std::string& pattern)
{
    if (pattern == "random") {
        add_task<worker_task_random>(new worker::worker_task_random());
    }
    update_state();
}

void block_worker::update_state()
{
    zmq_send(m_socket.get(), std::addressof(state), sizeof(state), 0);
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

} // namespace openperf::block::worker