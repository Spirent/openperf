#ifndef _OP_PACKETIO_GENERIC_DRIVER_HPP_
#define _OP_PACKETIO_GENERIC_DRIVER_HPP_

#include <any>
#include <memory>
#include <optional>
#include <vector>

#include "tl/expected.hpp"

#include "packetio/generic_port.hpp"

namespace openperf {
namespace packetio {
namespace driver {

typedef uint16_t (*tx_burst)(int id, uint32_t hash, void* items[], uint16_t nb_items);

class generic_driver {
public:
    template <typename Driver>
    generic_driver(Driver d)
        : m_self(std::make_unique<driver_model<Driver>>(std::move(d)))
    {}

    std::vector<std::string> port_ids() const
    {
        return m_self->port_ids();
    }

    std::optional<port::generic_port> port(std::string_view id) const
    {
        return m_self->port(id);
    }

    std::optional<int> port_index(std::string_view id) const
    {
        return m_self->port_index(id);
    }

    tl::expected<std::string, std::string> create_port(std::string_view id, const port::config_data& config)
    {
        return m_self->create_port(id, config);
    }

    tl::expected<void, std::string> delete_port(std::string_view id)
    {
        return m_self->delete_port(id);
    }

    void start_all_ports()
    {
        return m_self->start_all_ports();
    }

    void stop_all_ports()
    {
        return m_self->stop_all_ports();
    }

private:
    struct driver_concept {
        virtual ~driver_concept() = default;
        virtual std::vector<std::string> port_ids() const = 0;
        virtual std::optional<port::generic_port> port(std::string_view id) const = 0;
        virtual std::optional<int> port_index(std::string_view id) const = 0;
        virtual tl::expected<std::string, std::string> create_port(std::string_view id, const port::config_data& config) = 0;
        virtual tl::expected<void, std::string> delete_port(std::string_view id) = 0;
        virtual void start_all_ports() = 0;
        virtual void stop_all_ports() = 0;
    };

    template <typename Driver>
    struct driver_model final : driver_concept {
        driver_model(Driver d)
            : m_driver(std::move(d))
        {}

        std::vector<std::string> port_ids() const override
        {
            return m_driver.port_ids();
        }

        std::optional<port::generic_port> port(std::string_view id) const override
        {
            return m_driver.port(id);
        }

        std::optional<int> port_index(std::string_view id) const override
        {
            return m_driver.port_index(id);
        }

        tl::expected<std::string, std::string> create_port(std::string_view id, const port::config_data& config) override
        {
            return m_driver.create_port(id, config);
        }

        tl::expected<void, std::string> delete_port(std::string_view id) override
        {
            return m_driver.delete_port(id);
        }

        void start_all_ports() override
        {
            return m_driver.start_all_ports();
        }

        void stop_all_ports() override
        {
            return m_driver.stop_all_ports();
        }

        Driver m_driver;
    };

    std::unique_ptr<driver_concept> m_self;
};

std::unique_ptr<generic_driver> make();

}
}
}

#endif /* _OP_PACKETIO_GENERIC_DRIVER_HPP_ */
