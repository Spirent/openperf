#ifndef _OP_BLOCK_GENERATOR_RESULT_MODEL_HPP_
#define _OP_BLOCK_GENERATOR_RESULT_MODEL_HPP_

#include <chrono>
#include <string>
#include <memory>
#include "timesync/chrono.hpp"

namespace openperf::block::model {

using time_point = std::chrono::time_point<timesync::chrono::realtime>;

struct block_generator_statistics
{
    int64_t ops_target;
    int64_t ops_actual;
    int64_t bytes_target;
    int64_t bytes_actual;
    int64_t io_errors;
    int64_t latency;
    int64_t latency_min;
    int64_t latency_max;
};

class block_generator_result
{
public:
    block_generator_result() = default;
    block_generator_result(const block_generator_result&) = default;

    std::string get_id() const;
    void set_id(std::string_view value);

    bool is_active() const;
    void set_active(bool value);

    time_point get_timestamp() const;
    void set_timestamp(const time_point& value);

    block_generator_statistics get_read_stats() const;
    void set_read_stats(const block_generator_statistics& value);

    block_generator_statistics get_write_stats() const;
    void set_write_stats(const block_generator_statistics& value);

protected:
    std::string id;
    bool active;
    time_point timestamp;
    block_generator_statistics read_stats;
    block_generator_statistics write_stats;
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_GENERATOR_RESULT_MODEL_HPP_