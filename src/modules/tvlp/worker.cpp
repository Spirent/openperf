#include <chrono>
#include "worker.hpp"

namespace openperf::tvlp::internal {

constexpr duration THRESHOLD = 100ms;

tvlp_worker_t::tvlp_worker_t(const model::tvlp_module_profile_t& profile)
{
    m_profile = profile;
    m_state.state.store(model::READY);
    m_state.total_offset.store(0ms);
}

void tvlp_worker_t::start(time_point start_time)
{
    if (m_scheduler_thread.valid()) m_scheduler_thread.wait();
    m_state.total_offset = 0ms;
    m_state.state =
        start_time > ref_clock::now() ? model::COUNTDOWN : model::RUNNING;
    m_state.stopped = false;

    m_scheduler_thread = std::async(
        std::launch::async,
        [this](time_point tp, model::tvlp_module_profile_t p) {
            schedule(tp, p);
        },
        start_time,
        m_profile);
}

void tvlp_worker_t::stop() { m_state.stopped.store(true); }

void tvlp_worker_t::schedule(time_point start_time,
                             const model::tvlp_module_profile_t& profile)
{
    m_state.state.store(model::COUNTDOWN);

    for (auto now = ref_clock::now(); now < start_time;
         now = ref_clock::now()) {
        duration sleep_time = std::min(start_time - now, THRESHOLD);
        std::this_thread::sleep_for(sleep_time);
    }

    m_state.state.store(model::RUNNING);
    for (auto entry : profile) {
        auto create_result =
            send_create(entry.config, entry.resource_id.value());
        auto gen_id = create_result.value();
        auto end_time = ref_clock::now() + entry.length;
        auto stat_id = send_start(gen_id);
        for (auto now = ref_clock::now(); now < end_time;
             now = ref_clock::now()) {
            duration sleep_time = std::min(end_time - now, THRESHOLD);
            std::this_thread::sleep_for(sleep_time);
        }
        send_stat(stat_id.value());
        send_stop(gen_id);
        send_delete(gen_id);
    }
}

} // namespace openperf::tvlp::internal