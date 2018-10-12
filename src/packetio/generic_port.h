#ifndef _ICP_PACKETIO_GENERIC_PORT_H_
#define _ICP_PACKETIO_GENERIC_PORT_H_

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "json.hpp"
#include "tl/expected.hpp"

namespace swagger {
namespace v1 {
namespace model {
class Port;
}
}
}

namespace icp {
namespace packetio {
namespace port {

enum class link_status { LINK_UNKNOWN = 0,
                         LINK_UP,
                         LINK_DOWN };

std::string to_string(const link_status&);

enum class link_duplex { DUPLEX_UNKNOWN = 0,
                         DUPLEX_HALF,
                         DUPLEX_FULL };

std::string to_string(const link_duplex&);

enum class link_speed { SPEED_UNKNOWN =      0,
                        SPEED_10M     =     10,
                        SPEED_100M    =    100,
                        SPEED_1G      =   1000,
                        SPEED_2_5G    =   2500,
                        SPEED_5G      =   5000,
                        SPEED_10G     =  10000,
                        SPEED_20G     =  20000,
                        SPEED_25G     =  25000,
                        SPEED_40G     =  40000,
                        SPEED_50G     =  50000,
                        SPEED_56G     =  56000,
                        SPEED_100G    = 100000,
                        SPEED_200G    = 200000 };

struct stats_data {
    int64_t rx_packets;
    int64_t tx_packets;
    int64_t rx_bytes;
    int64_t tx_bytes;
    int64_t rx_errors;
    int64_t tx_errors;
};

struct dpdk_config {
    bool auto_negotiation;
    link_speed speed;
    link_duplex duplex;
};

enum class lag_mode { LAG_UNKNOWN = 0,
                      LAG_802_3_AD };

std::string to_string(const lag_mode&);

struct bond_config {
    lag_mode mode;
    std::vector<int> ports;
};

enum class config_type { DPDK = 0,
                         BOND = 1 };

typedef std::variant<dpdk_config,
                     bond_config> config_data;

void from_json(const nlohmann::json&, config_data&);

/**
 * A type erasing definition of a port.
 * This object contains the minimum interface required for an object
 * to be considered a "port" by the upper level packetio code.
 */
class generic_port {
public:
    template <typename Port>
    generic_port(Port port)
        : m_self(std::make_shared<port_model<Port>>(std::move(port)))
    {}

    int id() const
    {
        return m_self->id();
    }

    std::string kind() const
    {
        return m_self->kind();
    }

    link_status link() const
    {
        return m_self->link();
    }

    link_speed speed() const
    {
        return m_self->speed();
    }

    link_duplex duplex() const
    {
        return m_self->duplex();
    }

    stats_data stats() const
    {
        return m_self->stats();
    }

    config_data config() const
    {
        return m_self->config();
    }

    tl::expected<void, std::string> config(const config_data& c)
    {
        return m_self->config(c);
    }

private:
    struct port_concept {
        virtual ~port_concept() = default;
        virtual int id() const = 0;
        virtual std::string kind() const = 0;
        virtual link_status link() const = 0;
        virtual link_speed speed() const = 0;
        virtual link_duplex duplex() const = 0;
        virtual stats_data stats() const = 0;
        virtual config_data config() const = 0;
        virtual tl::expected<void, std::string> config(const config_data&) = 0;
    };

    template <typename Port>
    struct port_model final : port_concept {
        port_model(Port port)
            : m_port(std::move(port))
        {}

        int id() const override
        {
            return m_port.id();
        }

        std::string kind() const override
        {
            return m_port.kind();
        }

        link_status link() const override
        {
            return m_port.link();
        }

        link_speed speed() const override
        {
            return m_port.speed();
        }

        link_duplex duplex() const override
        {
            return m_port.duplex();
        }

        stats_data stats() const override
        {
            return m_port.stats();
        }

        config_data config() const override
        {
            return m_port.config();
        }

        tl::expected<void, std::string> config(const config_data &c) override
        {
            return m_port.config(c);
        }

        Port m_port;
    };

    std::shared_ptr<port_concept> m_self;
};

std::shared_ptr<swagger::v1::model::Port> make_swagger_port(const generic_port& port);

}
}
}

#endif /* _ICP_PACKETIO_GENERIC_PORT_H_ */
