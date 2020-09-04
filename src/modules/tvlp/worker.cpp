#include <chrono>
#include "worker.hpp"
#include "api/api_internal_client.hpp"
#include "pistache/http_defs.h"

namespace openperf::tvlp::internal::worker {

constexpr duration THRESHOLD = 100ms;

tvlp_worker_t::tvlp_worker_t(const model::tvlp_module_profile_t& profile)
    : m_profile(profile)
{
    m_state.state.store(model::READY);
    m_state.offset.store(duration::zero());
    m_result_atomic.store(&m_result);
}

tvlp_worker_t::~tvlp_worker_t() { stop(); }

void tvlp_worker_t::start(const realtime::time_point& start_time)
{
    if (m_scheduler_thread.valid()) m_scheduler_thread.wait();
    m_state.offset = duration::zero();
    m_state.state =
        start_time > ref_clock::now() ? model::COUNTDOWN : model::RUNNING;
    m_state.stopped = false;
    m_result.clear();
    m_scheduler_thread = std::async(
        std::launch::async,
        [this](realtime::time_point tp, model::tvlp_module_profile_t p) {
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
        auto res = const_cast<worker_future*>(&m_scheduler_thread)->get();
        const_cast<tvlp_worker_t*>(this)->m_error = res.error();
    }
    return m_error;
}

duration tvlp_worker_t::offset() const { return m_state.offset.load(); }

model::json_vector tvlp_worker_t::results() const
{
    if (m_state.state.load() == model::ERROR) return model::json_vector();
    return *m_result_atomic;
}

tl::expected<void, std::string>
tvlp_worker_t::schedule(realtime::time_point start_time,
                        const model::tvlp_module_profile_t& profile)
{
    model::json_vector results;

    m_state.state.store(model::COUNTDOWN);
    for (auto now = realtime::now(); now < start_time; now = realtime::now()) {
        if (m_state.stopped.load()) {
            m_state.state.store(model::READY);
            return {};
        }
        duration sleep_time = std::min(start_time - now, THRESHOLD);
        std::this_thread::sleep_for(sleep_time);
    }

    m_state.state.store(model::RUNNING);
    duration total_offset = duration::zero();
    for (auto entry : profile) {
        if (m_state.stopped.load()) {
            m_state.state.store(model::READY);
            return {};
        }

        // Create generator
        auto create_result = send_create(
            entry.config, (entry.resource_id) ? entry.resource_id.value() : "");
        if (!create_result) {
            m_state.state.store(model::ERROR);
            return tl::make_unexpected(create_result.error());
        }
        auto gen_id = create_result.value();
        auto end_time = ref_clock::now() + entry.length;

        // Start generator
        auto start_result = send_start(gen_id);
        if (!start_result) {
            m_state.state.store(model::ERROR);
            return tl::make_unexpected(start_result.error());
        }

        // Add new statistics
        results.push_back(start_result.value().second);
        *m_result_atomic = results;

        // Wait until profile entry done
        auto started = ref_clock::now();
        for (auto now = started; now < end_time; now = ref_clock::now()) {
            if (m_state.stopped.load()) {
                m_state.state.store(model::READY);
                return {};
            }

            // Update statistics
            auto stat_result = send_stat(start_result.value().first);
            if (!stat_result) {
                m_state.state.store(model::ERROR);
                return tl::make_unexpected(stat_result.error());
            }
            results.back() = stat_result.value();
            *m_result_atomic = results;

            m_state.offset.store(total_offset + now - started);
            duration sleep_time = std::min(end_time - now, THRESHOLD);
            std::this_thread::sleep_for(sleep_time);
        }

        total_offset += entry.length;
        m_state.offset.store(total_offset);

        // Stop generator
        if (auto res = send_stop(gen_id); !res) {
            m_state.state.store(model::ERROR);
            return tl::make_unexpected(res.error());
        }

        // Update statistics
        auto stat_result = send_stat(start_result.value().first);
        if (!stat_result) {
            m_state.state.store(model::ERROR);
            return tl::make_unexpected(stat_result.error());
        }
        results.back() = stat_result.value();
        *m_result_atomic = results;

        // Delete generator
        if (auto res = send_delete(gen_id); !res) {
            m_state.state.store(model::ERROR);
            return tl::make_unexpected(res.error());
        }
    }
    m_state.state.store(model::READY);
    return {};
}

tl::expected<stat_pair_t, std::string>
tvlp_worker_t::send_start(const std::string& id)
{
    auto result = openperf::api::client::internal_api_post(
        generator_endpoint() + "/" + id + "/start",
        "",
        INTERNAL_REQUEST_TIMEOUT);

    if (result.first != Pistache::Http::Code::Created)
        return tl::make_unexpected(result.second);
    auto stat = nlohmann::json::parse(result.second);
    return std::pair(stat.at("id"), stat);
}
tl::expected<void, std::string> tvlp_worker_t::send_stop(const std::string& id)
{
    auto result = openperf::api::client::internal_api_post(
        generator_endpoint() + "/" + id + "/stop",
        "",
        INTERNAL_REQUEST_TIMEOUT);
    if (result.first != Pistache::Http::Code::No_Content)
        return tl::make_unexpected(result.second);
    return {};
}
tl::expected<nlohmann::json, std::string>
tvlp_worker_t::send_stat(const std::string& id)
{
    auto result = openperf::api::client::internal_api_get(
        generator_results_endpoint() + "/" + id, INTERNAL_REQUEST_TIMEOUT);
    if (result.first != Pistache::Http::Code::Ok)
        return tl::make_unexpected(result.second);
    return nlohmann::json::parse(result.second);
}
tl::expected<void, std::string>
tvlp_worker_t::send_delete(const std::string& id)
{
    auto result = openperf::api::client::internal_api_del(
        generator_endpoint() + "/" + id, INTERNAL_REQUEST_TIMEOUT);
    if (result.first != Pistache::Http::Code::No_Content)
        return tl::make_unexpected(result.second);
    return {};
}

} // namespace openperf::tvlp::internal::worker
