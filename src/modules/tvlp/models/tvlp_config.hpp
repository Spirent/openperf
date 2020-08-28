#ifndef _OP_TVLP_CONFIG_MODEL_HPP_
#define _OP_TVLP_CONFIG_MODEL_HPP_

#include <optional>
#include <string>
#include <chrono>
#include "json.hpp"
#include "modules/timesync/chrono.hpp"

namespace openperf::tvlp::model {

using duration = std::chrono::nanoseconds;
using time_point = std::chrono::time_point<timesync::chrono::realtime>;

struct tvlp_profile_entry_t
{
    duration length;
    std::optional<std::string> resource_id;
    nlohmann::json config;
};

using tvlp_module_profile_t = std::vector<tvlp_profile_entry_t>;

struct tvlp_profile_t
{
    std::optional<tvlp_module_profile_t> block;
    std::optional<tvlp_module_profile_t> cpu;
    std::optional<tvlp_module_profile_t> memory;
    std::optional<tvlp_module_profile_t> packet;
};

enum tvlp_state_t { READY = 0, COUNTDOWN, RUNNING, ERROR };

class tvlp_configuration_t
{
public:
    tvlp_configuration_t() = default;
    tvlp_configuration_t(const tvlp_configuration_t&) = default;

    std::string id() const { return m_id; };
    void id(std::string_view value) { m_id = value; };

    tvlp_profile_t profile() const { return m_profile; };
    void profile(const tvlp_profile_t& value) { m_profile = value; };

    tvlp_state_t state() const { return m_state; };
    time_point start_time() const { return m_start_time; };
    duration total_length() const { return m_total_length; };
    duration current_offset() const { return m_current_offset; };

    std::string error() const { return m_error; };

protected:
    std::string m_id = "";
    tvlp_state_t m_state = READY;

    time_point m_start_time = time_point::min();
    duration m_total_length = duration::zero();
    duration m_current_offset = duration::zero();

    tvlp_profile_t m_profile;
    std::string m_error = "";
};

} // namespace openperf::tvlp::model

#endif // _OP_TVLP_CONFIG_MODEL_HPP_
