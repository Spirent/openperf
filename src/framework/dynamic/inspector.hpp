#ifndef _OP_DYNAMIC_POOL_HPP_
#define _OP_DYNAMIC_POOL_HPP_

#include "dynamic/api.hpp"
#include "dynamic/threshold.hpp"
#include "framework/core/op_uuid.hpp"
#include "framework/config/op_config_utils.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <string_view>

namespace openperf::dynamic {

using namespace std::chrono_literals;

class inspectable
{
public:
    using timestamp_t = std::chrono::system_clock::time_point;

    virtual ~inspectable() {}
    virtual double field(std::string_view) const = 0;
    virtual timestamp_t timestamp() const = 0;
};

class inspector
{
public:
    using inspectable_ptr = std::unique_ptr<inspectable>;
    using poll_function = std::function<inspectable_ptr(void)>;

    using config_t = dynamic::configuration::threshold;
    using config_list = std::vector<config_t>;
    using result_t = dynamic::results::threshold_result;
    using result_list = std::vector<result_t>;
    using duration = std::chrono::milliseconds;

private:
    // Attributes
    std::atomic_bool m_stop;
    duration m_interval = 250ms;

    poll_function m_poll;
    std::thread m_thread;

    result_list m_thresholds;
    inspectable_ptr m_last_stat = nullptr;

public:
    inspector();
    inspector(poll_function&&);

    duration interval() const { return m_interval; }
    void interval(const duration& i) { m_interval = i; }

    config_list config() const;

    void reset();
    void stop() { m_stop = true; }
    void start(const config_list&);
    void start();

    result_list result() const { return m_thresholds; }

private:
    void loop();
    void spin();
    void configure(const config_list&);

    int weight(const dynamic_argument&, const inspectable&);
    double delta(const dynamic_argument&, const inspectable&);
};

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_POOL_HPP_
