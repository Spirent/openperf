#ifndef _OP_PACKETIO_GENERIC_EVENT_LOOP_HPP_
#define _OP_PACKETIO_GENERIC_EVENT_LOOP_HPP_

#include <any>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <variant>

namespace openperf::packetio::event_loop {

class generic_event_loop;

using event_notifier = std::variant<void*, int>;
using event_handler = std::function<int(generic_event_loop&, std::any)>;
using delete_handler = std::function<void(std::any)>;

class generic_event_loop
{
public:
    template <typename EventLoop>
    generic_event_loop(EventLoop loop)
        : m_self(std::make_unique<event_loop_model<EventLoop>>(std::move(loop)))
    {}

    bool add_callback(std::string_view name,
                      event_notifier notify,
                      event_handler on_event,
                      std::any arg) noexcept
    {
        return (
            m_self->add_callback(name, notify, on_event, std::nullopt, arg));
    }

    bool add_callback(std::string_view name,
                      event_notifier notify,
                      event_handler on_event,
                      delete_handler on_delete,
                      std::any arg) noexcept
    {
        return (m_self->add_callback(name, notify, on_event, on_delete, arg));
    }

    void del_callback(event_notifier notify) noexcept
    {
        return (m_self->del_callback(notify));
    }

    template <typename EventLoop> EventLoop& get() const
    {
        if (std::type_index(typeid(EventLoop))
            == std::type_index(m_self->type_info())) {
            return (static_cast<event_loop_model<EventLoop>&>(*m_self).m_loop);
        }
        throw std::bad_cast();
    }

private:
    struct event_loop_concept
    {
        virtual ~event_loop_concept() = default;
        virtual bool add_callback(std::string_view name,
                                  event_notifier notify,
                                  event_handler on_event,
                                  std::optional<delete_handler> on_delete,
                                  std::any arg) noexcept = 0;
        virtual void del_callback(event_notifier notify) noexcept = 0;
        virtual const std::type_info& type_info() const = 0;
    };

    template <typename EventLoop>
    struct event_loop_model final : event_loop_concept
    {
        event_loop_model(EventLoop loop)
            : m_loop(std::move(loop))
        {}

        bool add_callback(std::string_view name,
                          event_notifier notify,
                          event_handler on_event,
                          std::optional<delete_handler> on_delete,
                          std::any arg) noexcept override
        {
            return (
                m_loop.add_callback(name, notify, on_event, on_delete, arg));
        }

        void del_callback(event_notifier notify) noexcept override
        {
            return (m_loop.del_callback(notify));
        }

        const std::type_info& type_info() const override
        {
            return (typeid(EventLoop));
        }

        EventLoop m_loop;
    };

    std::unique_ptr<event_loop_concept> m_self;
};

} // namespace openperf::packetio::event_loop

#endif /* _OP_PACKETIO_GENERIC_EVENT_LOOP_HPP_ */
