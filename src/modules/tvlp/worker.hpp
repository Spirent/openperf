#ifndef _OP_TVLP_WORKER_HPP_
#define _OP_TVLP_WORKER_HPP_

#include <atomic>
#include <vector>
#include <chrono>
#include <future>

#include "framework/generator/task.hpp"
#include "modules/timesync/chrono.hpp"
#include "models/tvlp_config.hpp"
#include "tl/expected.hpp"

namespace openperf::tvlp::internal::worker {

using namespace std::chrono_literals;
using time_point = std::chrono::time_point<timesync::chrono::realtime>;
using ref_clock = timesync::chrono::monotime;
using realtime = timesync::chrono::realtime;
using duration = std::chrono::nanoseconds;

struct tvlp_worker_state_t
{
    std::atomic<model::tvlp_state_t> state;
    std::atomic<duration> offset;
    std::atomic_bool stopped;
};

class tvlp_worker_t
{
    using worker_future = std::future<std::optional<std::string>>;

public:
    tvlp_worker_t() = delete;
    tvlp_worker_t(const tvlp_worker_t&) = delete;
    tvlp_worker_t(const model::tvlp_module_profile_t&);
    virtual ~tvlp_worker_t();

    void start(const time_point& start_time = realtime::now());
    void stop();
    model::tvlp_state_t state() const;
    std::optional<std::string> error() const;
    duration offset() const;

protected:
    std::optional<std::string>
    schedule(time_point start_time,
             const model::tvlp_module_profile_t& profile);
    virtual tl::expected<std::string, std::string>
    send_create(const nlohmann::json& config,
                const std::string& resource_id) = 0;
    virtual tl::expected<std::string, std::string>
    send_start(const std::string& id) = 0;
    virtual tl::expected<void, std::string>
    send_stop(const std::string& id) = 0;
    virtual tl::expected<std::string, std::string>
    send_stat(const std::string& id) = 0;
    virtual tl::expected<void, std::string>
    send_delete(const std::string& id) = 0;

    tvlp_worker_state_t m_state;
    std::string m_error;
    worker_future m_scheduler_thread;
    model::tvlp_module_profile_t m_profile;
};

} // namespace openperf::tvlp::internal::worker

#endif