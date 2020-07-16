#include "worker.hpp"

namespace openperf::framework::generator {

// Constructors & Destructor
worker::worker(worker&& w)
    : m_paused(w.m_paused.load())
    , m_finished(w.m_finished.load())
    , m_context(w.m_context)
    , m_stats_socket(std::move(w.m_stats_socket))
    , m_thread_name(std::move(w.m_thread_name))
{}

worker::worker(const std::weak_ptr<void>& context, const std::string& name)
    : m_paused(true)
    , m_finished(true)
    , m_context(context)
    , m_stats_socket(op_socket_get_client(
          m_context.lock().get(), ZMQ_PUB, STATISTICS_ENDPOINT))
    , m_thread_name(name)
{}

// Methods : public
void worker::stop() {}

void worker::pause()
{
    if (m_finished) return;
    m_paused = true;
}

void worker::resume()
{
    if (m_finished) return;
    m_paused = false;
}

} // namespace openperf::framework::generator
