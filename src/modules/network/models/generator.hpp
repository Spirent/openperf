#ifndef _OP_NETWORK_GENERATOR_MODEL_HPP_
#define _OP_NETWORK_GENERATOR_MODEL_HPP_

#include <cinttypes>
#include <string>
#include <vector>
#include "server.hpp"

namespace openperf::network::model {

struct ratio_t
{
    uint32_t reads;
    uint32_t writes;
};

struct generator_config
{
    uint64_t connections;
    uint64_t ops_per_connection;
    uint64_t reads_per_sec;
    uint64_t read_size;
    uint64_t writes_per_sec;
    uint64_t write_size;
    std::optional<ratio_t> ratio;
};

struct generator_target
{
    std::string host;
    in_port_t port;
    protocol_t protocol;
    std::optional<std::string> interface;
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
