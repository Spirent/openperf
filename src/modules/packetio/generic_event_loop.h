#ifndef _ICP_PACKETIO_GENERIC_EVENT_LOOP_H_
#define _ICP_PACKETIO_GENERIC_EVENT_LOOP_H_

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <variant>

namespace icp::packetio::event_loop {

class generic_event_loop;

using event_notifier = std::variant<void*, int>;
using callback_function = std::function<int(generic_event_loop&, std::any)>;

class generic_event_loop
{
public:
    template <typename EventLoop>
    generic_event_loop(EventLoop loop)
        : m_self(std::make_shared<event_loop_model<EventLoop>>(std::move(loop)))
    {}

    bool add_callback(std::string_view name,
                      event_notifier notify,
                      callback_function callback,
                      std::any arg) noexcept
    {
        return (m_self->add_callback(name, notify, callback, arg));
    }

    void del_callback(event_notifier notify) noexcept
    {
        return (m_self->del_callback(notify));
    }

private:
    struct event_loop_concept {
        virtual ~event_loop_concept() = default;
        virtual bool add_callback(std::string_view name,
                                  event_notifier notify,
                                  callback_function callback,
                                  std::any arg) noexcept = 0;
        virtual void del_callback(event_notifier notify) noexcept = 0;
    };

    template <typename EventLoop>
    struct event_loop_model final : event_loop_concept {
        event_loop_model(EventLoop loop)
            : m_loop(std::move(loop))
        {}

        bool add_callback(std::string_view name,
                          event_notifier notify,
                          callback_function callback,
                          std::any arg) noexcept
        {
            return (m_loop.add_callback(name, notify, callback, arg));
        }

        void del_callback(event_notifier notify) noexcept
        {
            return (m_loop.del_callback(notify));
        }

        EventLoop m_loop;
    };

    std::shared_ptr<event_loop_concept> m_self;
};

}

#endif /* _ICP_PACKETIO_GENERIC_EVENT_LOOP_H_ */
