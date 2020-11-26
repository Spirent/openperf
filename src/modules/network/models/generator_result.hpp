#ifndef _OP_NETWORK_GENERATOR_RESULT_MODEL_HPP_
#define _OP_NETWORK_GENERATOR_RESULT_MODEL_HPP_

#include <string_view>

#include "modules/timesync/chrono.hpp"
#include "modules/dynamic/api.hpp"

namespace openperf::network::model {

class generator_result
{
public:
    using time_point = timesync::chrono::realtime::time_point;
    using duration = std::chrono::nanoseconds;
    using optional_time_t = std::optional<duration>;

    struct load_stat_t
    {
        uint64_t ops_target;
        uint64_t ops_actual;
        uint64_t bytes_target;
        uint64_t bytes_actual;
        uint64_t io_errors;
        duration latency;
        optional_time_t latency_min;
        optional_time_t latency_max;
    };

    struct conn_stat_t
    {
        uint64_t attempted;
        uint64_t successful;
        uint64_t closed;
        uint64_t errors;
    };

protected:
    std::string m_id;
    std::string m_generator_id;
    bool m_active;
    time_point m_timestamp;
    time_point m_start_timestamp;
    load_stat_t m_read;
    load_stat_t m_write;
    conn_stat_t m_conn;
    dynamic::results m_dynamic_results;

public:
    generator_result() = default;
    generator_result(const generator_result&) = default;

    std::string id() const { return m_id; }
    std::string generator_id() const { return m_generator_id; }
    bool active() const { return m_active; }
    time_point timestamp() const { return m_timestamp; }
    time_point start_timestamp() const { return m_start_timestamp; }
    load_stat_t read_stats() const { return m_read; }
    load_stat_t write_stats() const { return m_write; }
    conn_stat_t conn_stats() const { return m_conn; }
    dynamic::results dynamic_results() const { return m_dynamic_results; }

    void id(std::string_view id) { m_id = id; }
    void generator_id(std::string_view id) { m_generator_id = id; }
    void active(bool value) { m_active = value; }
    void timestamp(const time_point& value) { m_timestamp = value; }
    void start_timestamp(const time_point& value) { m_start_timestamp = value; }
    void read_stats(const load_stat_t& s) { m_read = s; }
    void write_stats(const load_stat_t& s) { m_write = s; }
    void conn_stats(const conn_stat_t& s) { m_conn = s; }
    void dynamic_results(const dynamic::results& d) { m_dynamic_results = d; }
};

} // namespace openperf::network::model

#endif // _OP_NETWORK_GENERATOR_RESULT_MODEL_HPP_
