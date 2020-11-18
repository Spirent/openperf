#include <chrono>
#include "worker.hpp"
#include "api/api_internal_client.hpp"
#include "pistache/http_defs.h"

namespace openperf::tvlp::internal::worker {

constexpr model::duration THRESHOLD = 100ms;

tvlp_worker_t::tvlp_worker_t(void* context,
                             const std::string& endpoint,
                             const model::tvlp_profile_t::series& series)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, endpoint.data()))
    , m_series(series)
{
    m_state.state.store(model::READY);
    m_state.offset.store(model::duration::zero());
    m_result.store(new model::json_vector());
}

tvlp_worker_t::~tvlp_worker_t()
{
    stop();
    delete m_result.exchange(nullptr);
}

tl::expected<void, std::string>
tvlp_worker_t::start(const model::time_point& start_time,
                     const model::tvlp_start_t::start_t& start_config)
{
    auto state = m_state.state.load();
    if (state == model::RUNNING || state == model::COUNTDOWN) {
        return tl::make_unexpected("Worker is already in running state");
    }

    if (m_scheduler_thread.valid()) m_scheduler_thread.wait();
    m_state.offset = model::duration::zero();
    m_state.state = start_time > model::ref_clock::now() ? model::COUNTDOWN
                                                         : model::RUNNING;
    m_state.stopped = false;
    delete m_result.exchange(new model::json_vector());
    m_scheduler_thread = std::async(
        std::launch::async,
        [this](auto&& series, auto&& time, auto&& start) {
            return schedule(series, time, start);
        },
        m_series,
        start_time,
        start_config);

    return {};
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
        const_cast<tvlp_worker_t*>(this)->m_error =
            const_cast<tvlp_worker_t*>(this)->m_scheduler_thread.get().error();
    }
    return m_error;
}

model::duration tvlp_worker_t::offset() const { return m_state.offset.load(); }

model::json_vector tvlp_worker_t::results() const
{
    auto state = m_state.state.load();
    if (state == model::ERROR) return model::json_vector();

    return *m_result.load(std::memory_order_consume);
}

void tvlp_worker_t::store_results(const nlohmann::json& result,
                                  result_store_operation operation)
{
    auto original = m_result.load(std::memory_order_relaxed);
    model::json_vector* updated;
    switch (operation) {
    case result_store_operation::ADD: {
        updated = new model::json_vector(original->push_back(result));
        break;
    }
    case result_store_operation::UPDATE: {
        updated =
            new model::json_vector(original->set(original->size() - 1, result));
        break;
    }
    }

    delete m_result.exchange(updated, std::memory_order_release);
}

tl::expected<void, std::string>
tvlp_worker_t::schedule(const model::tvlp_profile_t::series& series,
                        const model::time_point& start_time,
                        const model::tvlp_start_t::start_t& start_config)
{
    using model::realtime;
    using model::ref_clock;

    m_state.state.store(model::COUNTDOWN);
    for (auto now = realtime::now(); now < start_time; now = realtime::now()) {
        if (m_state.stopped.load()) {
            m_state.state.store(model::READY);
            return {};
        }
        model::duration sleep_time = std::min(start_time - now, THRESHOLD);
        std::this_thread::sleep_for(sleep_time);
    }

    m_state.state.store(model::RUNNING);
    model::duration total_offset = model::duration::zero();
    for (const auto& entry : series) {
        auto entry_duration =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                entry.length * start_config.time_scale);

        if (m_state.stopped.load()) {
            m_state.state.store(model::READY);
            return {};
        }

        // Create generator
        auto create_result = send_create(entry, start_config.load_scale);
        if (!create_result) {
            m_state.state.store(model::ERROR);
            return tl::make_unexpected(create_result.error());
        }
        auto gen_id = create_result.value();
        auto end_time = ref_clock::now() + entry_duration;

        // Start generator
        auto start_result = send_start(gen_id, start_config.dynamic_results);
        if (!start_result) {
            m_state.state.store(model::ERROR);
            return tl::make_unexpected(start_result.error());
        }

        // Add new statistics
        store_results(start_result.value().second, result_store_operation::ADD);

        // Wait until series entry done
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
            store_results(stat_result.value(), result_store_operation::UPDATE);

            m_state.offset.store(total_offset + now - started);
            model::duration sleep_time = std::min(end_time - now, THRESHOLD);
            std::this_thread::sleep_for(sleep_time);
        }

        total_offset += entry_duration;
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
        store_results(stat_result.value(), result_store_operation::UPDATE);

        // Delete generator
        if (auto res = send_delete(gen_id); !res) {
            m_state.state.store(model::ERROR);
            return tl::make_unexpected(res.error());
        }
    }
    m_state.state.store(model::READY);
    return {};
}

using namespace openperf::message;
tl::expected<serialized_message, int>
tvlp_worker_t::submit_request(serialized_message request)
{
    if (auto error = send(m_socket.get(), std::move(request)); error != 0) {
        return tl::make_unexpected(error);
    }

    auto reply = recv(m_socket.get());
    if (!reply) { return tl::make_unexpected(reply.error()); }

    return std::move(*reply);
}

} // namespace openperf::tvlp::internal::worker
