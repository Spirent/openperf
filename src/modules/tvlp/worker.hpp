#ifndef _OP_TVLP_WORKER_HPP_
#define _OP_TVLP_WORKER_HPP_

#include <atomic>
#include <vector>
#include <chrono>
#include <future>
#include "json.hpp"

#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"
#include "models/tvlp_config.hpp"
#include "models/tvlp_result.hpp"
#include "tl/expected.hpp"

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

static const std::chrono::seconds INTERNAL_REQUEST_TIMEOUT = 5s;

class tvlp_worker_t
{
    using worker_future =
        std::future<tl::expected<model::json_vector, std::string>>;

public:
    tvlp_worker_t() = delete;
    tvlp_worker_t(const tvlp_worker_t&) = delete;
    explicit tvlp_worker_t(const model::tvlp_module_profile_t&);
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
    send_start(const std::string& id);
    virtual tl::expected<void, std::string> send_stop(const std::string& id);
    virtual tl::expected<nlohmann::json, std::string>
    send_stat(const std::string& id);
    virtual tl::expected<void, std::string> send_delete(const std::string& id);

    virtual std::string generator_endpoint() = 0;
    virtual std::string generator_results_endpoint() = 0;

private:
    void fetch_future_results();
    tl::expected<model::json_vector, std::string>
    schedule(realtime::time_point start_time,
             const model::tvlp_module_profile_t& profile);

    tvlp_worker_state_t m_state;
    std::string m_error;
    std::shared_ptr<model::json_vector> m_result_ptr;
    worker_future m_scheduler_thread;
    model::tvlp_module_profile_t m_profile;
};

} // namespace openperf::tvlp::internal::worker

#endif