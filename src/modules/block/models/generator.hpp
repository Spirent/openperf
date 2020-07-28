#ifndef _OP_BLOCK_GENERATOR_MODEL_HPP_
#define _OP_BLOCK_GENERATOR_MODEL_HPP_

#include <optional>
#include <string>
#include <string_view>

namespace openperf::block::model {

enum class block_generation_pattern { RANDOM, SEQUENTIAL, REVERSE };

struct block_generator_ratio
{
    uint32_t reads;
    uint32_t writes;
};

struct block_generator_config
{
    uint32_t queue_depth;
    uint32_t reads_per_sec;
    uint32_t read_size;
    uint32_t writes_per_sec;
    uint32_t write_size;
    std::optional<block_generator_ratio> ratio;
    block_generation_pattern pattern;
};

class block_generator
{
protected:
    std::string m_id;
    block_generator_config m_config;
    std::string m_resource_id;
    bool m_running;

public:
    block_generator() = default;
    block_generator(const block_generator&) = default;

    std::string get_id() const { return m_id; }
    void set_id(std::string_view value) { m_id = value; }

    block_generator_config get_config() const { return m_config; }
    void set_config(const block_generator_config& value) { m_config = value; }

    std::string get_resource_id() const { return m_resource_id; }
    void set_resource_id(std::string_view value) { m_resource_id = value; }

    bool is_running() const { return m_running; }
    void set_running(bool value) { m_running = value; }
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_GENERATOR_MODEL_HPP_