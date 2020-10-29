#ifndef _OP_NETWORK_GENERATOR_MODEL_HPP_
#define _OP_NETWORK_GENERATOR_MODEL_HPP_

#include <cinttypes>
#include <string>
#include <vector>
#include "server.hpp"

namespace openperf::network::model {

struct generator_config
{
    uint64_t connections;
    uint32_t ops_per_connection;
    uint32_t reads_per_sec;
    uint32_t read_size;
    uint32_t writes_per_sec;
    uint32_t write_size;
};

struct generator_target
{
    std::string host;
    uint64_t port;
    protocol_t protocol;
};

class generator
{
public:
    generator() = default;
    generator(const generator&) = default;
    virtual ~generator() = default;

    virtual std::string id() const { return m_id; }
    virtual void id(std::string_view id) { m_id = id; }

    virtual generator_config config() const { return m_config; }
    virtual void config(const generator_config& value) { m_config = value; }

    virtual generator_target target() const { return m_target; }
    virtual void target(const generator_target& value) { m_target = value; }

    virtual bool running() const { return m_running; }
    virtual void running(bool value) { m_running = value; }

protected:
    std::string m_id;
    generator_config m_config;
    generator_target m_target;
    bool m_running;
};

} // namespace openperf::network::model

#endif // _OP_NETWORK_GENERATOR_MODEL_HPP_
