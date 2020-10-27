#ifndef _OP_NETWORK_SERVER_MODEL_HPP_
#define _OP_NETWORK_SERVER_MODEL_HPP_

#include <cinttypes>
#include <string>
#include <vector>

namespace openperf::network::model {

class server
{
public:
    enum class protocol_t { TCP, UDP };

    server() = default;
    server(const server&) = default;
    virtual ~server() = default;

    virtual std::string id() const { return m_id; }
    virtual void id(std::string_view id) { m_id = id; }

    virtual int64_t port() const { return m_port; }
    virtual void port(int64_t p) { m_port = p; }

    virtual protocol_t protocol() const { return m_protocol; }
    virtual void protocol(protocol_t p) { m_protocol = p; }

protected:
    std::string m_id;
    int64_t m_port;
    protocol_t m_protocol;
};

} // namespace openperf::network::model

#endif // _OP_NETWORK_GENERATOR_MODEL_HPP_
