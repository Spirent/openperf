#ifndef _ICP_PACKETIO_GENERIC_WORKERS_H_
#define _ICP_PACKETIO_GENERIC_WORKERS_H_

#include <any>
#include <functional>
#include <memory>
#include <string>

#include "tl/expected.hpp"

#include "packetio/generic_driver.h"
#include "packetio/generic_event_loop.h"
#include "packetio/generic_sink.h"
#include "packetio/generic_source.h"

namespace icp::core {
class event_loop;
}

namespace icp::packetio::workers {

using transmit_function = uint16_t (*)(int id, uint32_t hash, void* items[], uint16_t nb_items);

enum class context { NONE = 0,
                     STACK };

class generic_workers
{
public:
    template <typename Workers>
    generic_workers(Workers s)
        : m_self(std::make_unique<workers_model<Workers>>(std::move(s)))
    {}

    unsigned get_rx_worker_count() const
    {
        return (m_self->get_rx_worker_count());
    }

    unsigned get_tx_worker_count() const
    {
        return (m_self->get_tx_worker_count());
    }

    transmit_function get_transmit_function(std::string_view port_id) const
    {
        return (m_self->get_transmit_function(port_id));
    }

    void add_interface(std::string_view port_id, std::any interface)
    {
        return (m_self->add_interface(port_id, interface));
    }

    void del_interface(std::string_view port_id, std::any interface)
    {
        return (m_self->del_interface(port_id, interface));
    }

    tl::expected<void, int> add_sink(std::string_view src_id,
                                     packets::generic_sink sink)
    {
        return (m_self->add_sink(src_id, sink));
    }

    void del_sink(std::string_view src_id, packets::generic_sink sink)
    {
        m_self->del_sink(src_id, sink);
    }

    tl::expected<void, int> add_source(std::string_view dst_id,
                                       packets::generic_source source)
    {
        return (m_self->add_source(dst_id, source));
    }

    void del_source(std::string_view dst_id, packets::generic_source source)
    {
        m_self->del_source(dst_id, source);
    }

    tl::expected<std::string, int> add_task(context ctx,
                                            std::string_view name,
                                            event_loop::event_notifier notify,
                                            event_loop::event_handler on_event,
                                            std::any arg)
    {
        return (m_self->add_task(ctx, name, notify, on_event, std::nullopt, arg));
    }

    tl::expected<std::string, int> add_task(context ctx,
                                            std::string_view name,
                                            event_loop::event_notifier notify,
                                            event_loop::event_handler on_event,
                                            event_loop::delete_handler on_delete,
                                            std::any arg)
    {
        return (m_self->add_task(ctx, name, notify, on_event, on_delete, arg));
    }

    void del_task(std::string_view task_id)
    {
        m_self->del_task(task_id);
    }

private:
    struct workers_concept {
        virtual ~workers_concept() = default;
        virtual unsigned get_rx_worker_count() const = 0;
        virtual unsigned get_tx_worker_count() const = 0;
        virtual transmit_function get_transmit_function(std::string_view port_id) const = 0;
        virtual void add_interface(std::string_view port_id, std::any interface) = 0;
        virtual void del_interface(std::string_view port_id, std::any interface) = 0;
        virtual tl::expected<void, int> add_sink(std::string_view src_id,
                                                 packets::generic_sink sink) = 0;
        virtual void del_sink(std::string_view src_id, packets::generic_sink sink) = 0;
        virtual tl::expected<void, int> add_source(std::string_view dst_id,
                                                   packets::generic_source source) = 0;
        virtual void del_source(std::string_view dst_id, packets::generic_source source) = 0;
        virtual tl::expected<std::string, int> add_task(context ctx,
                                                        std::string_view name,
                                                        event_loop::event_notifier notify,
                                                        event_loop::event_handler on_event,
                                                        std::optional<event_loop::delete_handler> on_delete,
                                                        std::any arg) = 0;
        virtual void del_task(std::string_view task_id) = 0;
    };

    template <typename Workers>
    struct workers_model final : workers_concept {
        workers_model(Workers s)
            : m_workers(std::move(s))
        {}

        unsigned get_rx_worker_count() const override
        {
            return (m_workers.get_rx_worker_count());
        }

        unsigned get_tx_worker_count() const override
        {
            return (m_workers.get_tx_worker_count());
        }

        transmit_function get_transmit_function(std::string_view port_id) const override
        {
            return (m_workers.get_transmit_function(port_id));
        }

        void add_interface(std::string_view port_id, std::any interface) override
        {
            return (m_workers.add_interface(port_id, interface));
        }

        void del_interface(std::string_view port_id, std::any interface) override
        {
            return (m_workers.del_interface(port_id, interface));
        }

        tl::expected<void, int> add_sink(std::string_view src_id,
                                         packets::generic_sink sink) override
        {
            return (m_workers.add_sink(src_id, sink));
        }

        void del_sink(std::string_view src_id, packets::generic_sink sink) override
        {
            m_workers.del_sink(src_id, sink);
        }

        tl::expected<void, int> add_source(std::string_view dst_id,
                                           packets::generic_source source) override
        {
            return (m_workers.add_source(dst_id, source));
        }

        void del_source(std::string_view dst_id, packets::generic_source source) override
        {
            m_workers.del_source(dst_id, source);
        }

        tl::expected<std::string, int> add_task(context ctx,
                                                std::string_view name,
                                                event_loop::event_notifier notify,
                                                event_loop::event_handler on_event,
                                                std::optional<event_loop::delete_handler> on_delete,
                                                std::any arg) override
        {
            return (m_workers.add_task(ctx, name, notify, on_event, on_delete, arg));
        }

        void del_task(std::string_view task_id) override
        {
            m_workers.del_task(task_id);
        }

        Workers m_workers;
    };

    std::unique_ptr<workers_concept> m_self;
};

std::unique_ptr<generic_workers> make(void*, icp::core::event_loop&, driver::generic_driver&);

}

#endif /* _ICP_PACKETIO_GENERIC_WORKERS_H_ */
