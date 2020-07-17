#include "controller.hpp"

namespace openperf::framework::generator {

// Constructors & Destructor
controller::controller(const std::string& name)
    : m_context(zmq_ctx_new())
    , m_control_endpoint("inproc://" + name + "-command")
    , m_statistics_endpoint("inproc://" + name + "-statistics")
    , m_control_socket(op_socket_get_server(
          m_context.get(), ZMQ_PUB, m_control_endpoint.c_str()))
    , m_statistics_socket(op_socket_get_server(
          m_context.get(), ZMQ_SUB, m_statistics_endpoint.c_str()))
    , m_thread_name(name)
{
    if (zmq_setsockopt(m_statistics_socket.get(), ZMQ_SUBSCRIBE, "", 0)) {
        OP_LOG(OP_LOG_ERROR,
               "Controller ZMQ socket %s subscribtion error: %s",
               m_statistics_endpoint.c_str(),
               zmq_strerror(errno));
    }
}

controller::~controller()
{
    send(internal::operation_t::STOP);
    m_stop = true;
    if (m_thread.joinable()) m_thread.join();
}

// Methods : private
void controller::send(internal::operation_t operation)
{
    auto result = zmq_send(
        m_control_socket.get(), &operation, sizeof(operation), ZMQ_DONTWAIT);

    if (result < 0 && errno != EAGAIN) {
        if (errno == ETERM) {
            OP_LOG(OP_LOG_DEBUG,
                   "Controller ZMQ socket %s terminated",
                   m_control_endpoint.c_str());
        } else {
            OP_LOG(OP_LOG_ERROR,
                   "Controller ZMQ socket %s send with error: %s",
                   m_control_endpoint.c_str(),
                   zmq_strerror(errno));
        }
    }
}

} // namespace openperf::framework::generator
