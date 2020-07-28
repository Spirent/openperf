#ifndef _OP_BLOCK_GENERATOR_RESULT_MODEL_HPP_
#define _OP_BLOCK_GENERATOR_RESULT_MODEL_HPP_

#include <chrono>
#include <string>
#include <memory>
#include "timesync/chrono.hpp"

namespace openperf::block::model {

using time_point = std::chrono::time_point<timesync::chrono::realtime>;
using duration = std::chrono::nanoseconds;

struct block_generator_statistics
{
    int64_t ops_target;
    int64_t ops_actual;
    int64_t bytes_target;
    int64_t bytes_actual;
    int64_t io_errors;
    duration latency;
    duration latency_min;
    duration latency_max;
};

class block_generator_result
{
protected:
    std::string m_id;
    std::string m_generator_id;
    bool m_active;
    time_point m_timestamp;
    block_generator_statistics m_read_stats;
    block_generator_statistics m_write_stats;

public:
    block_generator_result() = default;
    block_generator_result(const block_generator_result&) = default;

    std::string get_id() const { return m_id; }
    void set_id(std::string_view value) { m_id = value; }

    std::string get_generator_id() const { return m_generator_id; }
    void set_generator_id(std::string_view value) { m_generator_id = value; }

    bool is_active() const { return m_active; }
    void set_active(bool value) { m_active = value; }

    time_point get_timestamp() const { return m_timestamp; }
    void set_timestamp(const time_point& value) { m_timestamp = value; }

    block_generator_statistics get_read_stats() const { return m_read_stats; }
    void set_read_stats(const block_generator_statistics& value)
    {
        m_read_stats = value;
    }

    block_generator_statistics get_write_stats() const { return m_write_stats; }
    void set_write_stats(const block_generator_statistics& value)
    {
        m_write_stats = value;
    }
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_GENERATOR_RESULT_MODEL_HPP_