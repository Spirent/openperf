#ifndef _OP_PACKETIO_GENERIC_WORKERS_HPP_
#define _OP_PACKETIO_GENERIC_WORKERS_HPP_

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "tl/expected.hpp"

#include "packetio/generic_driver.hpp"
#include "packetio/generic_event_loop.hpp"
#include "packetio/generic_interface.hpp"
#include "packetio/generic_sink.hpp"
#include "packetio/generic_source.hpp"

namespace openperf::core {
class event_loop;
}

namespace openperf::packetio::workers {

using transmit_function = uint16_t (*)(int id,
                                       uint32_t hash,
                                       void* items[],
                                       uint16_t nb_items);

enum class context { NONE = 0, STACK };

class generic_workers
{
public:
    template <typename Workers>
    generic_workers(Workers s)
        : m_self(std::make_unique<workers_model<Workers>>(std::move(s)))
    {}

    std::vector<unsigned> get_rx_worker_ids(
        std::optional<std::string_view> obj_id = std::nullopt) const
    {
        return (m_self->get_rx_worker_ids(obj_id));
    }

    std::vector<unsigned> get_tx_worker_ids(
        std::optional<std::string_view> obj_id = std::nullopt) const
    {
        return (m_self->get_tx_worker_ids(obj_id));
    }

    transmit_function get_transmit_function(std::string_view port_id) const
    {
        return (m_self->get_transmit_function(port_id));
    }

    void add_interface(std::string_view port_id,
                       interface::generic_interface interface)
    {
        m_self->add_interface(port_id, interface);
    }

    void del_interface(std::string_view port_id,
                       interface::generic_interface interface)
    {
        m_self->del_interface(port_id, interface);
    }

    tl::expected<void, int> add_sink(std::string_view src_id,
                                     packet::generic_sink sink)
    {
        return (m_self->add_sink(src_id, sink));
    }

    void del_sink(std::string_view src_id, packet::generic_sink sink)
    {
        m_self->del_sink(src_id, sink);
    }

    tl::expected<void, int> add_source(std::string_view dst_id,
                                       packet::generic_source source)
    {
        return (m_self->add_source(dst_id, source));
    }

    void del_source(std::string_view dst_id, packet::generic_source source)
    {
        m_self->del_source(dst_id, source);
    }

    tl::expected<std::string, int> add_task(context ctx,
                                            std::string_view name,
                                            event_loop::event_notifier notify,
                                            event_loop::event_handler on_event,
                                            std::any arg)
    {
        return (m_self->add_task(ctx,
                                 name,
                                 notify,
                                 std::move(on_event),
                                 std::nullopt,
                                 std::move(arg)));
    }

    tl::expected<std::string, int>
    add_task(context ctx,
             std::string_view name,
             event_loop::event_notifier notify,
             event_loop::event_handler on_event,
             event_loop::delete_handler on_delete,
             std::any arg)
    {
        return (m_self->add_task(ctx,
                                 name,
                                 notify,
                                 std::move(on_event),
                                 std::move(on_delete),
                                 std::move(arg)));
    }

    void del_task(std::string_view task_id) { m_self->del_task(task_id); }

private:
    struct workers_concept
    {
        virtual ~workers_concept() = default;
        virtual std::vector<unsigned>
        get_rx_worker_ids(std::optional<std::string_view> obj_id) const = 0;
        virtual std::vector<unsigned>
        get_tx_worker_ids(std::optional<std::string_view> obj_id) const = 0;
        virtual transmit_function
        get_transmit_function(std::string_view port_id) const = 0;
        virtual void add_interface(std::string_view port_id,
                                   interface::generic_interface interface) = 0;
        virtual void del_interface(std::string_view port_id,
                                   interface::generic_interface interface) = 0;
        virtual tl::expected<void, int> add_sink(std::string_view src_id,
                                                 packet::generic_sink sink) = 0;
        virtual void del_sink(std::string_view src_id,
                              packet::generic_sink sink) = 0;
        virtual tl::expected<void, int>
        add_source(std::string_view dst_id, packet::generic_source source) = 0;
        virtual void del_source(std::string_view dst_id,
                                packet::generic_source source) = 0;
        virtual tl::expected<std::string, int>
        add_task(context ctx,
                 std::string_view name,
                 event_loop::event_notifier notify,
                 event_loop::event_handler on_event,
                 std::optional<event_loop::delete_handler> on_delete,
                 std::any arg) = 0;
        virtual void del_task(std::string_view task_id) = 0;
    };

    template <typename Workers> struct workers_model final : workers_concept
    {
        workers_model(Workers s)
            : m_workers(std::move(s))
        {}

        std::vector<unsigned>
        get_rx_worker_ids(std::optional<std::string_view> obj_id) const override
        {
            return (m_workers.get_rx_worker_ids(obj_id));
        }

        std::vector<unsigned>
        get_tx_worker_ids(std::optional<std::string_view> obj_id) const override
        {
            return (m_workers.get_tx_worker_ids(obj_id));
        }

        transmit_function
        get_transmit_function(std::string_view port_id) const override
        {
            return (m_workers.get_transmit_function(port_id));
        }

        void add_interface(std::string_view port_id,
                           interface::generic_interface interface) override
        {
            m_workers.add_interface(port_id, interface);
        }

        void del_interface(std::string_view port_id,
                           interface::generic_interface interface) override
        {
            m_workers.del_interface(port_id, interface);
        }

        tl::expected<void, int> add_sink(std::string_view src_id,
                                         packet::generic_sink sink) override
        {
            return (m_workers.add_sink(src_id, sink));
        }

        void del_sink(std::string_view src_id,
                      packet::generic_sink sink) override
        {
            m_workers.del_sink(src_id, sink);
        }

        tl::expected<void, int>
        add_source(std::string_view dst_id,
                   packet::generic_source source) override
        {
            return (m_workers.add_source(dst_id, source));
        }

        void del_source(std::string_view dst_id,
                        packet::generic_source source) override
        {
            m_workers.del_source(dst_id, source);
        }

        tl::expected<std::string, int>
        add_task(context ctx,
                 std::string_view name,
                 event_loop::event_notifier notify,
                 event_loop::event_handler on_event,
                 std::optional<event_loop::delete_handler> on_delete,
                 std::any arg) override
        {
            return (m_workers.add_task(ctx,
                                       name,
                                       notify,
                                       std::move(on_event),
                                       std::move(on_delete),
                                       std::move(arg)));
        }

        void del_task(std::string_view task_id) override
        {
            m_workers.del_task(task_id);
        }

        Workers m_workers;
    };

    std::unique_ptr<workers_concept> m_self;
};

std::unique_ptr<generic_workers>
make(void*, openperf::core::event_loop&, driver::generic_driver&);

} // namespace openperf::packetio::workers

#endif /* _OP_PACKETIO_GENERIC_WORKERS_HPP_ */
