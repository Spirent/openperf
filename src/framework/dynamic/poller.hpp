#ifndef _OP_DYNAMIC_POOL_HPP_
#define _OP_DYNAMIC_POOL_HPP_

#include "dynamic/api.hpp"
#include "dynamic/threshold.hpp"
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

    using threshold_result = dynamic::results::threshold_result;
    using threshold_result_list = std::vector<threshold_result>;

private:
    // Attributes
    std::atomic_bool m_stop;
    duration m_interval = 250ms;

    poll_function m_poll;
    std::thread m_thread;

    threshold_result_list m_thresholds;
    pollable_ptr m_last_stat = nullptr;

public:
    poller();
    poller(poll_function&&);

    dynamic::configuration config() const;
    dynamic::results result() const;

    duration interval() const { return m_interval; }
    void interval(const duration& i) { m_interval = i; }

    void reset();
    void stop() { m_stop = true; }
    void start(const dynamic::configuration&);
    void start();

private:
    void loop();
    void spin();
    void configure(const dynamic::configuration&);

    int weight(const dynamic::argument&, const pollable&);
    double delta(const dynamic::argument&, const pollable&);
};

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_POOL_HPP_
