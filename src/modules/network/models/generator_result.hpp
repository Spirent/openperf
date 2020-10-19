#ifndef _OP_NETWORK_GENERATOR_RESULT_MODEL_HPP_
#define _OP_NETWORK_GENERATOR_RESULT_MODEL_HPP_

#include <string_view>

#include "modules/timesync/chrono.hpp"
#include "modules/dynamic/api.hpp"

namespace openperf::network::model {

struct generator_stat
{};

class generator_result
{
private:
    using time_point = timesync::chrono::realtime::time_point;

protected:
    std::string m_id;
    std::string m_generator_id;
    bool m_active;
    time_point m_timestamp;
    time_point m_start_timestamp;
    generator_stat m_stats;
    dynamic::results m_dynamic_results;

public:
    generator_result() = default;
    generator_result(const generator_result&) = default;

    std::string id() const { return m_id; }
    std::string generator_id() const { return m_generator_id; }
    bool active() const { return m_active; }
    generator_stat stats() const { return m_stats; }
    time_point timestamp() const { return m_timestamp; }
    time_point start_timestamp() const { return m_start_timestamp; }
    dynamic::results dynamic_results() const { return m_dynamic_results; }

    void id(std::string_view id) { m_id = id; }
    void generator_id(std::string_view id) { m_generator_id = id; }
    void active(bool active) { m_active = active; }
    void stats(const generator_stat& stats) { m_stats = stats; }
    void timestamp(const time_point& time) { m_timestamp = time; }
    void start_timestamp(const time_point& time) { m_start_timestamp = time; }
    void dynamic_results(const dynamic::results& r) { m_dynamic_results = r; }
};

} // namespace openperf::network::model

#endif // _OP_NETWORK_GENERATOR_RESULT_MODEL_HPP_