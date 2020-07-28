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
    virtual ~block_generator() = default;

    virtual std::string id() const { return m_id; }
    virtual block_generator_config config() const { return m_config; }
    virtual std::string resource_id() const { return m_resource_id; }
    virtual bool is_running() const { return m_running; }

    virtual void id(std::string_view value) { m_id = value; }
    virtual void config(const block_generator_config& conf) { m_config = conf; }
    virtual void resource_id(std::string_view value) { m_resource_id = value; }
    virtual void running(bool value) { m_running = value; }
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_GENERATOR_MODEL_HPP_