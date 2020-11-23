#ifndef _OP_TVLP_CONFIG_MODEL_HPP_
#define _OP_TVLP_CONFIG_MODEL_HPP_

#include <chrono>
#include <optional>
#include <string>

#include "json.hpp"

#include "modules/dynamic/api.hpp"
#include "modules/timesync/chrono.hpp"

namespace openperf::tvlp::model {

using ref_clock = timesync::chrono::monotime;
using realtime = timesync::chrono::realtime;
using duration = std::chrono::nanoseconds;
using time_point = timesync::chrono::realtime::time_point;

struct tvlp_profile_t
{
    struct entry
    {
        duration length;
        std::optional<std::string> resource_id;
        nlohmann::json config;
    };

    using series = std::vector<entry>;

    std::optional<series> block;
    std::optional<series> cpu;
    std::optional<series> memory;
    std::optional<series> packet;
    std::optional<series> network;
};

struct tvlp_start_t
{
    struct start_t
    {
        double time_scale = 1.0;
        double load_scale = 1.0;
        dynamic::configuration dynamic_results;
    };

    time_point start_time = realtime::now();
    start_t block;
    start_t cpu;
    start_t memory;
    start_t packet;
};

enum tvlp_state_t { READY = 0, COUNTDOWN, RUNNING, ERROR };

class tvlp_configuration_t
{
protected:
    std::string m_id;
    tvlp_state_t m_state = READY;

    time_point m_start_time = time_point::min();
    duration m_total_length = duration::zero();
    duration m_current_offset = duration::zero();

    tvlp_profile_t m_profile;
    std::string m_error;

public:
    tvlp_configuration_t() = default;
    tvlp_configuration_t(const tvlp_configuration_t&) = default;

    std::string id() const { return m_id; }
    tvlp_profile_t profile() const { return m_profile; }
    tvlp_state_t state() const { return m_state; }
    time_point start_time() const { return m_start_time; }
    duration total_length() const { return m_total_length; }
    duration current_offset() const { return m_current_offset; }
    std::string error() const { return m_error; }

    void id(std::string_view value) { m_id = value; }
    void profile(const tvlp_profile_t& value) { m_profile = value; }
};

} // namespace openperf::tvlp::model

#endif // _OP_TVLP_CONFIG_MODEL_HPP_
