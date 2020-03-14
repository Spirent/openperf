#ifndef _OP_PACKETIO_GENERIC_INTERFACE_HPP_
#define _OP_PACKETIO_GENERIC_INTERFACE_HPP_

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "json.hpp"
#include "tl/expected.hpp"

#include "net/net_types.hpp"

namespace swagger {
namespace v1 {
namespace model {
class Interface;
}
} // namespace v1
} // namespace swagger

namespace openperf {
namespace packetio {
namespace interface {

struct stats_data
{
    int64_t rx_packets;
    int64_t tx_packets;
    int64_t rx_bytes;
    int64_t tx_bytes;
    int64_t rx_errors;
    int64_t tx_errors;
};

struct eth_protocol_config
{
    net::mac_address address;
};

struct ipv4_auto_protocol_config
{};

struct ipv4_dhcp_protocol_config
{
    std::optional<std::string> hostname;
    std::optional<std::string> client;
};

struct ipv4_static_protocol_config
{
    std::optional<net::ipv4_address> gateway;
    net::ipv4_address address;
    uint8_t prefix_length;
};

typedef std::variant<ipv4_auto_protocol_config,
                     ipv4_dhcp_protocol_config,
                     ipv4_static_protocol_config>
    ipv4_protocol_config;

struct ipv6_common_protocol_config
{
    std::optional<net::ipv6_address> link_local_address;
};

struct ipv6_auto_protocol_config : public ipv6_common_protocol_config
{};

struct ipv6_dhcp6_protocol_config : public ipv6_common_protocol_config
{
    bool stateless;
};

struct ipv6_static_protocol_config : public ipv6_common_protocol_config
{
    std::optional<net::ipv6_address> gateway;
    net::ipv6_address address;
    uint8_t prefix_length;
};

typedef std::variant<ipv6_auto_protocol_config,
                     ipv6_dhcp6_protocol_config,
                     ipv6_static_protocol_config>
    ipv6_protocol_config;

typedef std::variant<std::monostate,
                     eth_protocol_config,
                     ipv4_protocol_config,
                     ipv6_protocol_config>
    protocol_config;

struct config_data
{
    std::vector<protocol_config> protocols;
    std::string port_id;
    std::string id;
};

config_data make_config_data(const swagger::v1::model::Interface&);

class generic_interface
{
public:
    template <typename Interface>
    generic_interface(Interface intf)
        : m_self(std::make_shared<interface_model<Interface>>(std::move(intf)))
    {}

    std::string id() const { return m_self->id(); }

    std::string port_id() const { return m_self->port_id(); }

    std::string mac_address() const { return m_self->mac_address(); }

    std::optional<std::string> ipv4_address() const
    {
        return m_self->ipv4_address();
    }

    std::optional<std::string> ipv6_address() const
    {
        return m_self->ipv6_address();
    }

    std::optional<std::string> ipv6_linklocal_address() const
    {
        return m_self->ipv6_linklocal_address();
    }

    config_data config() const { return m_self->config(); }

    std::any data() const { return m_self->data(); }

    stats_data stats() const { return m_self->stats(); }

private:
    struct interface_concept
    {
        virtual ~interface_concept() = default;
        virtual std::string id() const = 0;
        virtual std::string port_id() const = 0;
        virtual std::string mac_address() const = 0;
        virtual std::optional<std::string> ipv4_address() const = 0;
        virtual std::optional<std::string> ipv6_address() const = 0;
        virtual std::optional<std::string> ipv6_linklocal_address() const = 0;
        virtual config_data config() const = 0;
        virtual std::any data() const = 0;
        virtual stats_data stats() const = 0;
    };

    template <typename Interface>
    struct interface_model final : interface_concept
    {
        interface_model(Interface intf)
            : m_interface(std::move(intf))
        {}

        std::string id() const override { return m_interface.id(); }

        std::string port_id() const override { return m_interface.port_id(); }

        std::string mac_address() const override
        {
            return m_interface.mac_address();
        }

        std::optional<std::string> ipv4_address() const override
        {
            return m_interface.ipv4_address();
        }

        std::optional<std::string> ipv6_address() const override
        {
            return m_interface.ipv6_address();
        }

        std::optional<std::string> ipv6_linklocal_address() const override
        {
            return m_interface.ipv6_linklocal_address();
        }

        config_data config() const override { return m_interface.config(); }

        std::any data() const override { return m_interface.data(); }

        stats_data stats() const override { return m_interface.stats(); }

        Interface m_interface;
    };

    std::shared_ptr<const interface_concept> m_self;
};

std::shared_ptr<swagger::v1::model::Interface>
make_swagger_interface(const generic_interface&);

template <typename T>
std::optional<T> get_optional_key(const nlohmann::json& j, const char* key)
{
    return (j.find(key) != j.end() && !j[key].is_null()
                ? std::make_optional(j[key].get<T>())
                : std::nullopt);
}

} // namespace interface
} // namespace packetio
} // namespace openperf

#endif /* _OP_PACKETIO_GENERIC_INTERFACE_HPP_ */
