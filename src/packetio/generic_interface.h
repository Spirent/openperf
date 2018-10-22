#ifndef _ICP_PACKETIO_GENERIC_INTERFACE_H_
#define _ICP_PACKETIO_GENERIC_INTERFACE_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "json.hpp"
#include "tl/expected.hpp"

#include "net/net_types.h"

namespace swagger {
namespace v1 {
namespace model {
class Interface;
}
}
}

namespace icp {
namespace packetio {
namespace interface {

struct stats_data {
    int64_t rx_packets;
    int64_t tx_packets;
    int64_t rx_bytes;
    int64_t tx_bytes;
    int64_t rx_errors;
    int64_t tx_errors;
};

struct eth_protocol_config {
    net::mac_address address;
};

struct ipv4_dhcp_protocol_config {
    std::optional<std::string> hostname;
    std::optional<std::string> client;
};

struct ipv4_static_protocol_config {
    std::optional<net::ipv4_address> gateway;
    net::ipv4_address address;
    uint8_t prefix_length;
};

typedef std::variant<ipv4_static_protocol_config,
                     ipv4_dhcp_protocol_config> ipv4_protocol_config;

typedef std::variant<eth_protocol_config,
                     ipv4_protocol_config> protocol_config;

struct config_data {
    std::vector<protocol_config> protocols;
    int port_id;
};

config_data make_config_data(const swagger::v1::model::Interface&);

class generic_interface {
public:
    template <typename Interface>
    generic_interface(Interface intf)
        : m_self(std::make_shared<interface_model<Interface>>(std::move(intf)))
    {}

    int id() const
    {
        return m_self->id();
    }

    int port_id() const
    {
        return m_self->port_id();
    }

    std::string mac_address() const
    {
        return m_self->mac_address();
    }

    std::string ipv4_address() const
    {
        return m_self->ipv4_address();
    }

    stats_data stats() const
    {
        return m_self->stats();
    }

    config_data config() const
    {
        return m_self->config();
    }

private:
    struct interface_concept {
        virtual ~interface_concept() = default;
        virtual int id() const = 0;
        virtual int port_id() const = 0;
        virtual std::string mac_address() const = 0;
        virtual std::string ipv4_address() const = 0;
        virtual stats_data stats() const = 0;
        virtual config_data config() const = 0;
    };

    template <typename Interface>
    struct interface_model final : interface_concept {
        interface_model(Interface intf)
            : m_interface(std::move(intf))
        {}

        int id() const override
        {
            return m_interface.id();
        }

        int port_id() const override
        {
            return m_interface.port_id();
        }

        std::string mac_address() const override
        {
            return m_interface.mac_address();
        }

        std::string ipv4_address() const override
        {
            return m_interface.ipv4_address();
        }

        stats_data stats() const override
        {
            return m_interface.stats();
        }

        config_data config() const override
        {
            return m_interface.config();
        }

        Interface m_interface;
    };

    std::shared_ptr<interface_concept> m_self;
};

std::shared_ptr<swagger::v1::model::Interface> make_swagger_interface(const generic_interface&);

template <typename T>
std::optional<T> get_optional_key(const nlohmann::json& j, const char *key)
{
    return (j.find(key) != j.end() && !j[key].is_null()
            ? std::make_optional(j[key].get<T>())
            : std::nullopt);
}

}
}
}

#endif /* _ICP_PACKETIO_GENERIC_INTERFACE_H_ */
