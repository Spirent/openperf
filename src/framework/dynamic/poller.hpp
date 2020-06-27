#ifndef _OP_DYNAMIC_POOL_HPP_
#define _OP_DYNAMIC_POOL_HPP_

#include "dynamic/api.hpp"
#include "framework/core/op_uuid.hpp"
#include "framework/config/op_config_utils.hpp"
#include "modules/timesync/chrono.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <string_view>

namespace openperf::dynamic {

using namespace std::chrono_literals;

class pollable
{
public:
    using timestamp_t = openperf::timesync::chrono::realtime::time_point;

    virtual ~pollable() {}
    virtual double field(std::string_view) const = 0;
    virtual bool has_field(std::string_view) const = 0;
    virtual timestamp_t timestamp() const = 0;
};

class poller
{
public:
    using pollable_ptr = std::unique_ptr<pollable>;
    using poll_function = std::function<pollable_ptr(void)>;
    using duration = std::chrono::milliseconds;

    using threshold_result = dynamic::results::threshold;
    using threshold_result_list = std::vector<threshold_result>;
    using tdigest_result = dynamic::results::tdigest;
    using tdigest_result_list = std::vector<tdigest_result>;

private:
    // Attributes
    std::atomic_bool m_stop;
    duration m_interval = 250ms;

    poll_function m_poll;
    std::thread m_thread;

    threshold_result_list m_thresholds;
    tdigest_result_list m_tdigests;
    pollable_ptr m_last_stat = nullptr;

public:
    poller();
    poller(poll_function&&);

    dynamic::configuration config() const;
    dynamic::results result() const;

    duration interval() const { return m_interval; }
    void interval(const duration& i) { m_interval = i; }

    void reset();
    void stop();
    void start(const dynamic::configuration&);
    void start();

private:
    void loop();
    void spin();
    void configure(const dynamic::configuration&);

    int weight(const dynamic::argument_t&, const pollable&);
    double delta(const dynamic::argument_t&, const pollable&);

    void argument_check(const dynamic::argument_t&) const;
};

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_POOL_HPP_
