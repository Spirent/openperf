#ifndef _OP_PACKETIO_GENERIC_INTERFACE_HPP_
#define _OP_PACKETIO_GENERIC_INTERFACE_HPP_

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "json.hpp"
#include "tl/expected.hpp"

#include "packet/type/net_types.hpp"

namespace swagger::v1::model {
class Interface;
} // namespace swagger::v1::model

namespace openperf::packetio::interface {

struct stats_data
{
    int64_t rx_packets;
    int64_t tx_packets;
    int64_t rx_bytes;
    int64_t tx_bytes;
    int64_t rx_errors;
    int64_t tx_errors;
};

/*
 * DHCP client states as specified in RFC 2131
 */
enum class dhcp_client_state {
    none = 0,
    rebooting,
    init_reboot,
    init,
    selecting,
    requesting,
    checking,
    bound,
    renewing,
    rebinding,
};

enum class ipv6_address_state {
    none = 0,
    invalid,
    tentative,
    preferred,
    deprecated,
    duplicated
};

struct eth_protocol_config
{
    libpacket::type::mac_address address;
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
    std::optional<libpacket::type::ipv4_address> gateway;
    libpacket::type::ipv4_address address;
    uint8_t prefix_length;
};

using ipv4_protocol_config = std::variant<ipv4_auto_protocol_config,
                                          ipv4_dhcp_protocol_config,
                                          ipv4_static_protocol_config>;

struct ipv6_common_protocol_config
{
    std::optional<libpacket::type::ipv6_address> link_local_address;
};

struct ipv6_auto_protocol_config : public ipv6_common_protocol_config
{};

struct ipv6_dhcp6_protocol_config : public ipv6_common_protocol_config
{
    bool stateless;
};

struct ipv6_static_protocol_config : public ipv6_common_protocol_config
{
    std::optional<libpacket::type::ipv6_address> gateway;
    libpacket::type::ipv6_address address;
    uint8_t prefix_length;
};

using ipv6_protocol_config = std::variant<ipv6_auto_protocol_config,
                                          ipv6_dhcp6_protocol_config,
                                          ipv6_static_protocol_config>;

using protocol_config = std::variant<std::monostate,
                                     eth_protocol_config,
                                     ipv4_protocol_config,
                                     ipv6_protocol_config>;

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

    dhcp_client_state dhcp_state() const { return m_self->dhcp_state(); }

    ipv6_address_state ipv6_state() const { return m_self->ipv6_state(); }

    std::string mac_address() const { return m_self->mac_address(); }

    std::optional<std::string> ipv4_address() const
    {
        return m_self->ipv4_address();
    }

    std::optional<std::string> ipv4_gateway() const
    {
        return m_self->ipv4_gateway();
    }

    std::optional<uint8_t> ipv4_prefix_length() const
    {
        return m_self->ipv4_prefix_length();
    }

    std::optional<std::string> ipv6_address() const
    {
        return m_self->ipv6_address();
    }

    std::optional<uint8_t> ipv6_scope() const { return m_self->ipv6_scope(); }

    const std::optional<std::string> ipv6_linklocal_address() const
    {
        return m_self->ipv6_linklocal_address();
    }

    config_data config() const { return m_self->config(); }

    std::any data() const { return m_self->data(); }

    stats_data stats() const { return m_self->stats(); }

    /* Note: this function is called from a worker thread */
    int input_packet(void* packet) const
    {
        return m_self->input_packet(packet);
    }

    bool operator==(const generic_interface& other) const
    {
        return (id() == other.id());
    }

private:
    struct interface_concept
    {
        virtual ~interface_concept() = default;
        virtual std::string id() const = 0;
        virtual std::string port_id() const = 0;
        virtual std::string mac_address() const = 0;
        virtual dhcp_client_state dhcp_state() const = 0;
        virtual ipv6_address_state ipv6_state() const = 0;
        virtual std::optional<std::string> ipv4_address() const = 0;
        virtual std::optional<std::string> ipv4_gateway() const = 0;
        virtual std::optional<uint8_t> ipv4_prefix_length() const = 0;
        virtual std::optional<std::string> ipv6_address() const = 0;
        virtual std::optional<uint8_t> ipv6_scope() const = 0;
        virtual std::optional<std::string> ipv6_linklocal_address() const = 0;
        virtual config_data config() const = 0;
        virtual std::any data() const = 0;
        virtual stats_data stats() const = 0;
        virtual int input_packet(void* packet) const = 0;
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

        dhcp_client_state dhcp_state() const override
        {
            return m_interface.dhcp_state();
        }

        ipv6_address_state ipv6_state() const override
        {
            return m_interface.ipv6_state();
        }

        std::optional<std::string> ipv4_address() const override
        {
            return m_interface.ipv4_address();
        }

        std::optional<std::string> ipv4_gateway() const override
        {
            return m_interface.ipv4_gateway();
        }

        std::optional<uint8_t> ipv4_prefix_length() const override
        {
            return m_interface.ipv4_prefix_length();
        }

        std::optional<std::string> ipv6_address() const override
        {
            return m_interface.ipv6_address();
        }

        std::optional<uint8_t> ipv6_scope() const override
        {
            return m_interface.ipv6_scope();
        }

        std::optional<std::string> ipv6_linklocal_address() const override
        {
            return m_interface.ipv6_linklocal_address();
        }

        config_data config() const override { return m_interface.config(); }

        std::any data() const override { return m_interface.data(); }

        stats_data stats() const override { return m_interface.stats(); }

        int input_packet(void* packet) const override
        {
            return (m_interface.input_packet(packet));
        }

        Interface m_interface;
    };

    std::shared_ptr<const interface_concept> m_self;
};

std::unique_ptr<swagger::v1::model::Interface>
make_swagger_interface(const generic_interface&);

bool is_valid(const swagger::v1::model::Interface&, std::vector<std::string>&);

template <typename T>
std::optional<T> get_optional_key(const nlohmann::json& j, const char* key)
{
    return (j.find(key) != j.end() && !j[key].is_null()
                ? std::make_optional(j[key].get<T>())
                : std::nullopt);
}

} // namespace openperf::packetio::interface

#endif /* _OP_PACKETIO_GENERIC_INTERFACE_HPP_ */
