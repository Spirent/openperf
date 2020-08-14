#include <chrono>
#include "worker.hpp"

namespace openperf::tvlp::internal::worker {

constexpr duration THRESHOLD = 100ms;

tvlp_worker_t::tvlp_worker_t(const model::tvlp_module_profile_t& profile)
    : m_error("")
    , m_profile(profile)
{
    m_state.state.store(model::READY);
    m_state.offset.store(duration::zero());
}

tvlp_worker_t::~tvlp_worker_t() { stop(); }

void tvlp_worker_t::start(const time_point& start_time)
{
    if (m_scheduler_thread.valid()) m_scheduler_thread.wait();
    m_state.offset = duration::zero();
    m_state.state =
        start_time > ref_clock::now() ? model::COUNTDOWN : model::RUNNING;
    m_state.stopped = false;

    m_scheduler_thread = std::async(
        std::launch::async,
        [this](time_point tp, model::tvlp_module_profile_t p) {
            return schedule(tp, p);
        },
        start_time,
        m_profile);
}

void tvlp_worker_t::stop()
{
    if (m_scheduler_thread.valid()) {
        m_state.stopped.store(true);
        m_scheduler_thread.wait();
    }
    m_state.state.store(model::READY);
}

model::tvlp_state_t tvlp_worker_t::state() const
{
    return m_state.state.load();
}

std::optional<std::string> tvlp_worker_t::error() const
{
    if (m_state.state.load() != model::ERROR) return std::nullopt;
    if (m_scheduler_thread.valid()) {
        auto err = const_cast<worker_future*>(&m_scheduler_thread)->get();
        assert(err.has_value());
        const_cast<tvlp_worker_t*>(this)->m_error = err.value();
    }
    return m_error;
}

duration tvlp_worker_t::offset() const { return m_state.offset.load(); }

std::optional<std::string>
tvlp_worker_t::schedule(time_point start_time,
                        const model::tvlp_module_profile_t& profile)
{
    m_state.state.store(model::COUNTDOWN);
    for (auto now = realtime::now(); now < start_time; now = realtime::now()) {
        if (m_state.stopped.load()) {
            m_state.state.store(model::READY);
            return std::nullopt;
        }
        duration sleep_time = std::min(start_time - now, THRESHOLD);
        std::this_thread::sleep_for(sleep_time);
    }

    m_state.state.store(model::RUNNING);
    duration total_offset = duration::zero();
    for (auto entry : profile) {
        if (m_state.stopped.load()) {
            m_state.state.store(model::READY);
            return std::nullopt;
        }

        // Create generator
        auto create_result =
            send_create(entry.config, entry.resource_id.value());
        if (!create_result) {
            m_state.state.store(model::ERROR);
            return create_result.error();
        }
        auto gen_id = create_result.value();
        auto end_time = ref_clock::now() + entry.length;

        // Start generator
        auto start_result = send_start(gen_id);
        if (!start_result) {
            m_state.state.store(model::ERROR);
            return start_result.error();
        }

        // Wait until profile entry done
        auto started = ref_clock::now();
        for (auto now = started; now < end_time; now = ref_clock::now()) {
            if (m_state.stopped.load()) {
                m_state.state.store(model::READY);
                return std::nullopt;
            }
            m_state.offset.store(total_offset + now - started);
            duration sleep_time = std::min(end_time - now, THRESHOLD);
            std::this_thread::sleep_for(sleep_time);
        }

        total_offset += entry.length;
        m_state.offset.store(total_offset);

        // Stop generator
        if (auto res = send_stop(gen_id); !res) {
            m_state.state.store(model::ERROR);
            return res.error();
        }

        // Recv statistics
        send_stat(start_result.value());

        // Delete generator
        if (auto res = send_delete(gen_id); !res) {
            m_state.state.store(model::ERROR);
            return res.error();
        }
    }
    m_state.state.store(model::READY);
    return std::nullopt;
}

} // namespace openperf::tvlp::internal::worker