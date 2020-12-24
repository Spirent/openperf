#include "worker.hpp"

namespace openperf::framework::generator::internal {

/**
 * Note: It is advisable to replace ZMQ_PUB/ZMQ_SUB to
 * ZMQ_RADIO/ZMQ_DISH when the latter gets the stable status.
 */

// Constructors & Destructor
worker::worker(socket_pointer&& control_socket,
               socket_pointer&& statistics_socket,
               const std::string& name)
    : m_control_socket(std::move(control_socket))
    , m_statistics_socket(std::move(statistics_socket))
    , m_thread_name(name)
    , m_finished(true)
{}

worker::~worker()
{
    if (m_thread.joinable()) m_thread.join();
}

// Methods : private
void worker::send(operation_t op)
{
    auto msg = message::serialized_message{};
    message::push(msg, op);

    if (auto r = message::send(m_statistics_socket.get(), std::move(msg)); r) {
        OP_LOG(OP_LOG_ERROR,
               "Worker ZMQ statistics socket send with error: %s",
               zmq_strerror(r));
    }
}

operation_t worker::next_command(bool wait) noexcept
{
    constexpr int ZMQ_WAIT = 0;
    auto operation = operation_t::NOOP;
    auto recv = zmq_recv(m_control_socket.get(),
                         &operation,
                         sizeof(operation),
                         wait ? ZMQ_WAIT : ZMQ_DONTWAIT);

    if (recv < 0 && errno != EAGAIN) {
        operation = operation_t::STOP;
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG, "Worker ZMQ control socket terminated");
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "Worker ZMQ control socket receive with error: %s",
                   zmq_strerror(errno));
        }
    }

    return operation;
}

} // namespace openperf::framework::generator::internal
