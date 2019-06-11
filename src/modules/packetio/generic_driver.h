#ifndef _ICP_PACKETIO_GENERIC_DRIVER_H_
#define _ICP_PACKETIO_GENERIC_DRIVER_H_

#include <any>
#include <memory>
#include <optional>
#include <vector>

#include "tl/expected.hpp"

#include "packetio/generic_port.h"
#include "packetio/pga/generic_sink.h"
#include "packetio/pga/generic_source.h"

namespace icp {
namespace packetio {
namespace driver {

typedef uint16_t (*tx_burst)(int id, uint32_t hash, void* items[], uint16_t nb_items);

class generic_driver {
public:
    template <typename Driver>
    generic_driver(Driver d)
        : m_self(std::make_unique<driver_model<Driver>>(std::move(d)))
    {}

    std::vector<int> port_ids() const
    {
        return m_self->port_ids();
    }

    std::optional<port::generic_port> port(int id) const
    {
        return m_self->port(id);
    }

    tx_burst tx_burst_function(int port) const
    {
        return m_self->tx_burst_function(port);
    }

    tl::expected<int, std::string> create_port(const port::config_data& config)
    {
        return m_self->create_port(config);
    }

    tl::expected<void, std::string> delete_port(int id)
    {
        return m_self->delete_port(id);
    }

    tl::expected<void, int> attach_port_sink(std::string_view id, pga::generic_sink& sink)
    {
        return m_self->attach_port_sink(id, sink);
    }

    void detach_port_sink(std::string_view id, pga::generic_sink& sink)
    {
        m_self->detach_port_sink(id, sink);
    }

    tl::expected<void, int> attach_port_source(std::string_view id, pga::generic_source& source)
    {
        return m_self->attach_port_source(id, source);
    }

    void detach_port_source(std::string_view id, pga::generic_source& source)
    {
        m_self->detach_port_source(id, source);
    }

    void add_interface(int id, std::any interface)
    {
        m_self->add_interface(id, std::move(interface));
    }

    void del_interface(int id, std::any interface)
    {
        m_self->del_interface(id, std::move(interface));
    }

private:
    struct driver_concept {
        virtual ~driver_concept() = default;
        virtual std::vector<int> port_ids() const = 0;
        virtual std::optional<port::generic_port> port(int id) const = 0;
        virtual tx_burst tx_burst_function(int port) const = 0;
        virtual tl::expected<int, std::string> create_port(const port::config_data& config) = 0;
        virtual tl::expected<void, std::string> delete_port(int id) = 0;
        virtual tl::expected<void, int> attach_port_sink(std::string_view id, pga::generic_sink& sink) = 0;
        virtual void detach_port_sink(std::string_view id, pga::generic_sink& sink) = 0;
        virtual tl::expected<void, int> attach_port_source(std::string_view id, pga::generic_source& source) = 0;
        virtual void detach_port_source(std::string_view id, pga::generic_source& source) = 0;
        virtual void add_interface(int id, std::any interface) = 0;
        virtual void del_interface(int id, std::any interface) = 0;
    };

    template <typename Driver>
    struct driver_model final : driver_concept {
        driver_model(Driver d)
            : m_driver(std::move(d))
        {}

        std::vector<int> port_ids() const override
        {
            return m_driver.port_ids();
        }

        std::optional<port::generic_port> port(int id) const override
        {
            return m_driver.port(id);
        }

        tx_burst tx_burst_function(int port) const override
        {
            return m_driver.tx_burst_function(port);
        }

        tl::expected<int, std::string> create_port(const port::config_data& config) override
        {
            return m_driver.create_port(config);
        }

        tl::expected<void, std::string> delete_port(int id) override
        {
            return m_driver.delete_port(id);
        }

        tl::expected<void, int> attach_port_sink(std::string_view id, pga::generic_sink& sink) override
        {
            return m_driver.attach_port_sink(id, sink);
        }

        void detach_port_sink(std::string_view id, pga::generic_sink& sink) override
        {
            m_driver.detach_port_sink(id, sink);
        }

        tl::expected<void, int> attach_port_source(std::string_view id, pga::generic_source& source) override
        {
            return m_driver.attach_port_source(id, source);
        }

        void detach_port_source(std::string_view id, pga::generic_source& source) override
        {
            m_driver.detach_port_source(id, source);
        }

        void add_interface(int id, std::any interface) override
        {
            return m_driver.add_interface(id, std::move(interface));
        }

        void del_interface(int id, std::any interface) override
        {
            return m_driver.del_interface(id, std::move(interface));
        }

        Driver m_driver;
    };

    std::unique_ptr<driver_concept> m_self;
};

std::unique_ptr<generic_driver> make(void*);

}
}
}

#endif /* _ICP_PACKETIO_GENERIC_DRIVER_H_ */
