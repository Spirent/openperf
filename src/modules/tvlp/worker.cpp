#include <chrono>
#include "worker.hpp"
#include "api/api_internal_client.hpp"
#include "pistache/http_defs.h"

namespace openperf::tvlp::internal::worker {

using namespace std::chrono_literals;

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
    m_state.state =
        start_time > model::realtime::now() ? model::COUNTDOWN : model::RUNNING;
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

tl::expected<tvlp_worker_t::start_result_t, std::string>
tvlp_worker_t::send_toggle(const std::string&,
                           const std::string&,
                           const dynamic::configuration&)
{
    return tl::make_unexpected("Toggle is not supported");
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

template <typename ToClock, typename FromClock>
std::chrono::time_point<ToClock>
clock_cast(const std::chrono::time_point<FromClock>& point)
{
    auto from_now = FromClock::now().time_since_epoch();
    auto to_now = ToClock::now().time_since_epoch();
    return std::chrono::time_point<ToClock>(
        to_now + (point.time_since_epoch() - from_now));
}

tl::expected<void, std::string>
tvlp_worker_t::schedule(const model::tvlp_profile_t::series& profile,
                        const model::time_point& start_time,
                        const model::tvlp_start_t::start_t& start_config)
{
    using model::realtime;
    using model::ref_clock;
    entry_state_t entry_state = {};

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
    for (const auto& entry : profile) {
        bool last_entry = (&entry == &profile.back());

        if (m_state.stopped.load()) {
            if (entry_state.entry) { do_entry_stop(entry_state); }
            m_state.state.store(model::READY);
            return {};
        }

        if (auto res = do_entry_start(entry_state, entry, start_config); !res) {
            if (entry_state.entry) { do_entry_stop(entry_state); }
            m_state.state.store(model::ERROR);
            return res;
        }

        // Calculate end time of profile
        auto entry_duration =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                entry.length * start_config.time_scale);
        auto end_time = entry_state.start_time + entry_duration;

        // Wait until profile entry done
        for (auto now = entry_state.start_time; now < end_time;
             now = ref_clock::now()) {
            m_state.offset.store(total_offset + now - entry_state.start_time);

            if (m_state.stopped.load()) {
                do_entry_stop(entry_state);
                m_state.state.store(model::READY);
                return {};
            }

            // Update statistics
            if (auto res = do_entry_stats(entry_state); !res) {
                do_entry_stop(entry_state);
                m_state.state.store(model::ERROR);
                return res;
            }

            model::duration sleep_time = std::min(end_time - now, THRESHOLD);
            std::this_thread::sleep_for(sleep_time);
        }

        total_offset += entry_duration;
        m_state.offset.store(total_offset);

        if (!supports_toggle() || last_entry) {
            if (auto res = do_entry_stop(entry_state); !res) {
                m_state.state.store(model::ERROR);
                return res;
            }
        }
    }
    m_state.state.store(model::READY);
    return {};
}

tl::expected<void, std::string>
tvlp_worker_t::do_entry_start(entry_state_t& state,
                              const model::tvlp_profile_t::entry& entry,
                              const model::tvlp_start_t::start_t& start_config)
{
    // Create generator
    auto create_result = send_create(entry, start_config.load_scale);
    if (!create_result) { return tl::make_unexpected(create_result.error()); }
    auto gen_id = create_result.value();

    if (!state.entry) {
        // Start generator
        auto start_result = send_start(gen_id, start_config.dynamic_results);
        if (!start_result) {
            send_delete(gen_id);
            return tl::make_unexpected(start_result.error());
        }
        state.entry = &entry;
        state.gen_id = gen_id;
        state.result_id = start_result.value().result_id;
        state.start_time =
            clock_cast<model::ref_clock>(start_result.value().start_time);

        // Add new statistics
        store_results(start_result.value().statistics,
                      result_store_operation::ADD);
    } else {
        auto toggle_result =
            send_toggle(state.gen_id, gen_id, start_config.dynamic_results);
        if (!toggle_result) {
            send_delete(gen_id);
            return tl::make_unexpected(toggle_result.error());
        }
        auto prev_state = state;
        state.entry = &entry;
        state.gen_id = gen_id;
        state.result_id = toggle_result.value().result_id;
        state.start_time =
            clock_cast<model::ref_clock>(toggle_result.value().start_time);

        // Update statistics from previously active generator
        auto stat_result = do_entry_stats(prev_state);

        // Add new statistics
        store_results(toggle_result.value().statistics,
                      result_store_operation::ADD);

        // Delete previous generator
        if (auto res = send_delete(prev_state.gen_id); !res) { return res; }

        // Also an error if didn't get last stats
        if (!stat_result) { return stat_result; }
    }
    return {};
}

tl::expected<void, std::string>
tvlp_worker_t::do_entry_stop(entry_state_t& state)
{
    // Stop generator
    if (auto res = send_stop(state.gen_id); !res) { return res; }

    if (auto res = do_entry_stats(state); !res) { return res; }

    // Delete generator
    if (auto res = send_delete(state.gen_id); !res) { return res; }

    state.entry = nullptr;
    state.gen_id.clear();
    state.result_id.clear();

    return {};
}

tl::expected<void, std::string>
tvlp_worker_t::do_entry_stats(entry_state_t& state)
{
    // Update statistics
    auto stat_result = send_stat(state.result_id);
    if (!stat_result) { return tl::make_unexpected(stat_result.error()); }
    store_results(stat_result.value(), result_store_operation::UPDATE);

    return {};
}

tl::expected<message::serialized_message, int>
tvlp_worker_t::submit_request(message::serialized_message request)
{
    if (auto error = send(m_socket.get(), std::move(request)); error != 0) {
        return tl::make_unexpected(error);
    }

    auto reply = message::recv(m_socket.get());
    if (!reply) { return tl::make_unexpected(reply.error()); }

    return std::move(*reply);
}

} // namespace openperf::tvlp::internal::worker
