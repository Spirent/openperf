#ifndef _OP_NETWORK_SERVER_MODEL_HPP_
#define _OP_NETWORK_SERVER_MODEL_HPP_

#include <cinttypes>
#include <string>
#include <vector>
#include <netinet/in.h>

namespace openperf::network::model {

enum protocol_t { TCP, UDP };

class server
{
public:
    struct stat_t
    {
        uint64_t bytes_sent = 0;
        uint64_t bytes_received = 0;
        uint64_t connections = 0;
        uint64_t closed = 0;
        uint64_t errors = 0;
    };

    server() = default;
    server(const server& s)
        : m_id(s.m_id)
        , m_port(s.m_port)
        , m_protocol(s.m_protocol)
        , m_stat(s.stat()){};
    virtual ~server() = default;

    virtual std::string id() const { return m_id; }
    virtual void id(std::string_view id) { m_id = id; }

    virtual in_port_t port() const { return m_port; }
    virtual void port(in_port_t p) { m_port = p; }

    virtual protocol_t protocol() const { return m_protocol; }
    virtual void protocol(protocol_t p) { m_protocol = p; }

    virtual stat_t stat() const { return m_stat; }
    virtual void stat(stat_t p) { m_stat = p; }

protected:
    std::string m_id;
    in_port_t m_port;
    protocol_t m_protocol;
    stat_t m_stat;
};

} // namespace openperf::network::model

#endif // _OP_NETWORK_GENERATOR_MODEL_HPP_
