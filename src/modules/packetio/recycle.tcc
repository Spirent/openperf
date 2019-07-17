#include "core/icp_log.h"
#include "packetio/recycle.h"

namespace icp::packetio::recycle {

template <int NumReaders>
depot<NumReaders>::depot()
    : m_writer_version(base_version)
{
    for (auto& state : m_reader_states) {
        state.version.store(idle_version);
    }
}

template <int NumReaders>
void depot<NumReaders>::writer_add_reader(size_t reader_id)
{
    m_active_readers.set(reader_id);
}

template <int NumReaders>
void depot<NumReaders>::writer_del_reader(size_t reader_id)
{
    m_active_readers.set(reader_id, false);
}

template <int NumReaders>
void depot<NumReaders>::writer_add_gc_callback(const std::function<void()>& callback)
{
    auto v = m_writer_version.load(std::memory_order_relaxed);
    auto [it, success] = m_callbacks.emplace(v, std::move(callback));
    if (!success) {
        ICP_LOG(ICP_LOG_ERROR, "Failed to add garbage collecting callback for version %zu\n", v);
        return;
    }
    m_writer_version.fetch_add(1, std::memory_order_acq_rel);
}

template <int NumReaders>
void depot<NumReaders>::writer_add_gc_callback(std::function<void()>&& callback)
{
    auto v = m_writer_version.load(std::memory_order_relaxed);
    auto [it, success] = m_callbacks.emplace(v, std::forward<gc_callback>(callback));
    if (!success) {
        ICP_LOG(ICP_LOG_ERROR, "Failed to add garbage collecting callback for version %zu\n", v);
        return;
    }
    m_writer_version.fetch_add(1, std::memory_order_acq_rel);
}

template <int NumReaders>
void depot<NumReaders>::writer_process_gc_callbacks()
{
    /* Obviously, no reader should be at a higher version than our writer */
    size_t min_version = m_writer_version.load(std::memory_order_relaxed);
    for (size_t idx = 0; idx < m_active_readers.size(); idx++) {
        /* Skip unregistered readers */
        if (!m_active_readers.test(idx)) continue;

        /*
         * Check the reader version; if it's not idle, then use it for the
         * min version calculation.
         */
        if (auto reader_v = m_reader_states[idx].version.load(std::memory_order_consume);
            reader_v != idle_version) {
            min_version = std::min(min_version, reader_v);
        }
    }

    /* Find and run all callbacks between [0, min_version) */
    std::for_each(std::begin(m_callbacks), m_callbacks.lower_bound(min_version),
                  [](auto& pair) {
                      ICP_LOG(ICP_LOG_DEBUG, "Running garbage collection callback for version %zu\n",
                              pair.first);
                      pair.second();
                  });

    /* And delete them */
    m_callbacks.erase(std::begin(m_callbacks),
                      m_callbacks.lower_bound(min_version));
}

template <int NumReaders>
void depot<NumReaders>::reader_checkpoint(size_t reader_id)
{
    auto v = m_writer_version.load(std::memory_order_consume);
    m_reader_states[reader_id].version.store(v, std::memory_order_release);
}

template <int NumReaders>
void depot<NumReaders>::reader_idle(size_t reader_id)
{
    m_reader_states[reader_id].version.store(idle_version, std::memory_order_release);
}

template <int NumReaders>
guard<NumReaders>::guard(depot<NumReaders>& depot, size_t reader_id)
    : m_depot(depot)
    , m_id(reader_id)
{
    m_depot.reader_checkpoint(m_id);
}

template <int NumReaders>
guard<NumReaders>::~guard()
{
    m_depot.reader_idle(m_id);
}

}
