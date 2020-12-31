#include "controller.hpp"

namespace openperf::framework::generator {

// Constructors & Destructor
controller::controller(const std::string& name)
    : m_context(zmq_ctx_new())
    , m_control_endpoint("inproc://" + name + "-command")
    , m_statistics_endpoint("inproc://" + name + "-statistics")
    , m_thread_name(name)
{
    // Set ZMQ threads to 0 for context
    // Note: Value 0 valid only for inproc:// transport
    if (zmq_ctx_set(m_context.get(), ZMQ_IO_THREADS, 0)) {
        OP_LOG(OP_LOG_ERROR,
               "Controller ZMQ set IO threads to 0: %s",
               zmq_strerror(errno));
    }

    m_control_socket.reset(op_socket_get_server(
        m_context.get(), ZMQ_PUB, m_control_endpoint.c_str()));

    m_statistics_socket.reset(op_socket_get_server(
        m_context.get(), ZMQ_SUB, m_statistics_endpoint.c_str()));

    if (zmq_setsockopt(m_statistics_socket.get(), ZMQ_SUBSCRIBE, "", 0)) {
        OP_LOG(OP_LOG_ERROR,
               "Controller ZMQ statistics socket %s subscribtion error: %s",
               m_statistics_endpoint.c_str(),
               zmq_strerror(errno));
    }
}

controller::~controller()
{
    clear();
    m_stop = true;
    zmq_ctx_shutdown(m_context.get());
    if (m_thread.joinable()) m_thread.join();
}

// Methods : public
void controller::clear()
{
    send(internal::operation_t::STOP);
    m_workers.clear();
    m_worker_count = 0;
}

// Methods : private
std::optional<message::serialized_message> controller::receive(bool wait)
{
    constexpr int ZMQ_WAIT = 0;
    auto recv = message::recv(m_statistics_socket.get(),
                              wait ? ZMQ_WAIT : ZMQ_DONTWAIT);

    if (!recv && recv.error() != EAGAIN) {
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG,
                   "Controller ZMQ statistics socket %s terminated",
                   m_statistics_endpoint.c_str());
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "Controller ZMQ statistics socket %s receive with error: %s",
                   m_statistics_endpoint.c_str(),
                   zmq_strerror(recv.error()));
        }

        throw std::runtime_error(zmq_strerror(recv.error()));
    }

    return recv ? std::make_optional(std::move(recv.value())) : std::nullopt;
}

void controller::send(internal::operation_t operation, bool wait)
{
    assert(!m_stop);

    // Prevent sending commands to the ZMQ socket without subscribers.
    // This resolve the bug with generators hanging on deletion.
    if (m_workers.empty()) return;

    if (wait) {
        m_feedback_operation = operation;
        m_feedback.init(m_worker_count);
    }

    auto result = zmq_send(
        m_control_socket.get(), &operation, sizeof(operation), ZMQ_DONTWAIT);

    if (result < 0 && errno != EAGAIN) {
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG,
                   "Controller ZMQ control socket %s terminated",
                   m_control_endpoint.c_str());
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "Controller ZMQ control socket %s send with error: %s",
                   m_control_endpoint.c_str(),
                   zmq_strerror(errno));
        }
    }

    if (wait) { m_feedback.wait(); }
}

} // namespace openperf::framework::generator
