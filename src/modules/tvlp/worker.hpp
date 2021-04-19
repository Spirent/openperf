#ifndef _OP_TVLP_WORKER_HPP_
#define _OP_TVLP_WORKER_HPP_

#include <atomic>
#include <vector>
#include <chrono>
#include <future>

#include "json.hpp"
#include "tl/expected.hpp"

#include "framework/core/op_core.h"
#include "framework/generator/task.hpp"
#include "framework/message/serialized_message.hpp"
#include "modules/timesync/chrono.hpp"
#include "tvlp/models/tvlp_config.hpp"
#include "tvlp/models/tvlp_result.hpp"

namespace openperf::tvlp::internal::worker {

struct tvlp_worker_state_t
{
    std::atomic<model::tvlp_state_t> state;
    std::atomic<model::duration> offset;
    std::atomic_bool stopped;
};

class tvlp_worker_t
{
    using worker_future = std::future<tl::expected<void, std::string>>;

protected:
    struct start_result_t
    {
        std::string result_id;
        nlohmann::json statistics;
        model::realtime::time_point start_time = model::realtime::now();
    };

public:
    tvlp_worker_t() = delete;
    tvlp_worker_t(const tvlp_worker_t&) = delete;
    explicit tvlp_worker_t(void*,
                           const std::string&,
                           const model::tvlp_profile_t::series&);
    virtual ~tvlp_worker_t();

    tl::expected<void, std::string> start(const model::time_point& start_time,
                                          const model::tvlp_start_t::start_t&);
    void stop();

    model::tvlp_state_t state() const;
    std::optional<std::string> error() const;
    model::duration offset() const;
    model::json_vector results() const;

protected:
    virtual tl::expected<std::string, std::string>
    send_create(const model::tvlp_profile_t::entry&, double load_scale) = 0;
    virtual tl::expected<start_result_t, std::string>
    send_start(const std::string& id,
               const dynamic::configuration& dynamic_results = {}) = 0;
    virtual tl::expected<void, std::string>
    send_stop(const std::string& id) = 0;
    virtual tl::expected<nlohmann::json, std::string>
    send_stat(const std::string& id) = 0;
    virtual tl::expected<void, std::string>
    send_delete(const std::string& id) = 0;
    virtual bool supports_toggle() const { return false; }
    virtual tl::expected<start_result_t, std::string>
    send_toggle(const std::string& out_id,
                const std::string& in_id,
                const dynamic::configuration& = {});

    tl::expected<message::serialized_message, int>
    submit_request(message::serialized_message request);

    std::unique_ptr<void, op_socket_deleter> m_socket;

private:
    struct entry_state_t
    {
        const model::tvlp_profile_t::entry* entry;
        model::ref_clock::time_point start_time;
        std::string gen_id;
        std::string result_id;
    };

    tl::expected<void, std::string>
    schedule(const model::tvlp_profile_t::series& profile,
             const model::time_point& time,
             const model::tvlp_start_t::start_t& start_config);

    tl::expected<void, std::string>
    do_entry_start(entry_state_t& state,
                   const model::tvlp_profile_t::entry& entry,
                   const model::tvlp_start_t::start_t& start_config);
    tl::expected<void, std::string> do_entry_stop(entry_state_t& state);
    tl::expected<void, std::string> do_entry_stats(entry_state_t& state);

    tvlp_worker_state_t m_state;
    std::string m_error;
    std::atomic<model::json_vector*> m_result;
    worker_future m_scheduler_thread;
    model::tvlp_profile_t::series m_series;

    enum class result_store_operation { ADD = 0, UPDATE };
    void store_results(const nlohmann::json& result,
                       result_store_operation operation);
};

} // namespace openperf::tvlp::internal::worker

#endif
