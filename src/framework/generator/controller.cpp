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
}

// Methods : private
void controller::send(internal::operation_t operation)
{
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
}

} // namespace openperf::framework::generator
