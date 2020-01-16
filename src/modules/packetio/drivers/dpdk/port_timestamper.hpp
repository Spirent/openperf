#ifndef _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_
#define _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_

#include <map>
#include <variant>

struct rte_eth_rxtx_callback;

namespace openperf::packetio::dpdk::port {

class callback_timestamper
{
public:
    callback_timestamper(uint16_t port_id);
    ~callback_timestamper();

    callback_timestamper(callback_timestamper&&);
    callback_timestamper& operator=(callback_timestamper&&);

    uint16_t port_id() const;

    void enable();
    void disable();

    using callback_map = std::map<uint16_t, const rte_eth_rxtx_callback*>;

private:
    callback_map m_callbacks;
    uint16_t m_port;
};

class null_timestamper
{
public:
    null_timestamper(uint16_t port_id);

    uint16_t port_id() const;

    void enable();
    void disable();

private:
    uint16_t m_port;
};

class timestamper
{
public:
    timestamper(uint16_t port_id);

    uint16_t port_id() const;

    void enable();
    void disable();

    using timestamper_strategy =
        std::variant<callback_timestamper, null_timestamper>;

private:
    timestamper_strategy m_timestamper;
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_ */
