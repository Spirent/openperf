#include "worker.hpp"
#include "core/op_core.h"
#include "zmq.h"

namespace openperf::block::worker 
{

block_worker::block_worker(const worker_config& p_config, bool p_running):
    running(p_running),
    config(p_config),
    m_context(zmq_ctx_new()),
    m_socket(op_socket_get_server(m_context, ZMQ_REP, endpoint_prefix.data()))
{
    auto p_thread = new std::thread([this]() {
        worker_loop(m_context);
    });
    worker_tread = std::unique_ptr<std::thread>(p_thread);

    zmq_send(m_socket.get(), std::addressof(p_config), sizeof(p_config), 0);
}

block_worker::~block_worker() 
{
    zmq_ctx_shutdown(m_context);
    worker_tread->detach();
    tasks.clear();
}

void block_worker::set_running(bool p_running) 
{
    running = p_running;
}

void block_worker::worker_loop(void* context)
{
    auto socket = std::unique_ptr<void, op_socket_deleter>(op_socket_get_client(context, ZMQ_REP, endpoint_prefix.data()));
    for (;;)
    {
        printf("%d\n", running);
        worker_config msg;
        int recv = zmq_recv(socket.get(), &msg, sizeof(msg), running ? ZMQ_NOBLOCK : 0);
        if (recv < 0 && errno != EAGAIN) {
            printf("Shutdown\n");
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
    printf("DONE\n");
}

} // namespace openperf::block::worker