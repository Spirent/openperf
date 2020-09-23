#ifndef _OP_TVLP_WORKER_HPP_
#define _OP_TVLP_WORKER_HPP_

#include <atomic>
#include <vector>
#include <chrono>
#include <future>
#include "json.hpp"

#include "framework/core/op_core.h"
#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"
#include "models/tvlp_config.hpp"
#include "models/tvlp_result.hpp"
#include "tl/expected.hpp"
#include "message/serialized_message.hpp"

namespace openperf::tvlp::internal::worker {

using namespace std::chrono_literals;
using ref_clock = timesync::chrono::monotime;
using realtime = timesync::chrono::realtime;
using duration = std::chrono::nanoseconds;
using stat_pair_t = std::pair<std::string, nlohmann::json>;

struct tvlp_worker_state_t
{
    std::atomic<model::tvlp_state_t> state;
    std::atomic<duration> offset;
    std::atomic_bool stopped;
};

class tvlp_worker_t
{
    using worker_future = std::future<tl::expected<void, std::string>>;

public:
    tvlp_worker_t() = delete;
    tvlp_worker_t(const tvlp_worker_t&) = delete;
    explicit tvlp_worker_t(void*,
                           const std::string&,
                           const model::tvlp_module_profile_t&);
    virtual ~tvlp_worker_t();

    tl::expected<void, std::string>
    start(const realtime::time_point& start_time = realtime::now());
    void stop();
    model::tvlp_state_t state() const;
    std::optional<std::string> error() const;
    duration offset() const;
    model::json_vector results() const;

protected:
    virtual tl::expected<std::string, std::string>
    send_create(const nlohmann::json& config,
                const std::string& resource_id) = 0;
    virtual tl::expected<stat_pair_t, std::string>
    send_start(const std::string& id) = 0;
    virtual tl::expected<void, std::string>
    send_stop(const std::string& id) = 0;
    virtual tl::expected<nlohmann::json, std::string>
    send_stat(const std::string& id) = 0;
    virtual tl::expected<void, std::string>
    send_delete(const std::string& id) = 0;

    tl::expected<message::serialized_message, int>
    submit_request(message::serialized_message request);

    std::unique_ptr<void, op_socket_deleter> m_socket;

private:
    tl::expected<void, std::string>
    schedule(realtime::time_point start_time,
             const model::tvlp_module_profile_t& profile);

    tvlp_worker_state_t m_state;
    std::string m_error;
    std::atomic<model::json_vector*> m_result;
    worker_future m_scheduler_thread;
    model::tvlp_module_profile_t m_profile;

    enum class result_store_operation { ADD = 0, UPDATE };
    void store_results(const nlohmann::json& result,
                       result_store_operation operation);
};

} // namespace openperf::tvlp::internal::worker

#endif
