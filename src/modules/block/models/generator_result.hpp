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
    block_generator_statistics m_read;
    block_generator_statistics m_write;

public:
    block_generator_result() = default;
    block_generator_result(const block_generator_result&) = default;

    std::string id() const { return m_id; }
    std::string generator_id() const { return m_generator_id; }
    bool is_active() const { return m_active; }
    time_point timestamp() const { return m_timestamp; }
    block_generator_statistics read_stats() const { return m_read; }
    block_generator_statistics write_stats() const { return m_write; }

    void id(std::string_view id) { m_id = id; }
    void generator_id(std::string_view id) { m_generator_id = id; }
    void active(bool value) { m_active = value; }
    void timestamp(const time_point& value) { m_timestamp = value; }
    void read_stats(const block_generator_statistics& s) { m_read = s; }
    void write_stats(const block_generator_statistics& s) { m_write = s; }
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_GENERATOR_RESULT_MODEL_HPP_