#ifndef _ICP_PACKETIO_DPDK_POLLABLE_EVENT_TCC_
#define _ICP_PACKETIO_DPDK_POLLABLE_EVENT_TCC_

#include <cassert>
#include <cstring>
#include <type_traits>

#include <sys/epoll.h>

#include "packetio/drivers/dpdk/dpdk.h"
#include "core/icp_log.h"

namespace icp::packetio::dpdk {

template <typename Derived>
class pollable_event
{
public:
    pollable_event() = default;

    /* We can't be moved as we pass a portion of our innards into the DPDK */
    pollable_event(pollable_event&&) = delete;
    pollable_event& operator=(pollable_event&&) = delete;

    using event_callback = void (*)(int, void*);

    bool add(int poll_fd, void* data)
    {
        /*
         * Make sure the event always has the correct data before use.  The
         * DPDK functions will wipe the struct when/if we remove the event.
         */
        m_event = rte_epoll_event{
            .epdata = {
                .event = EPOLLIN | EPOLLET,
                .data = data,
                .cb_fun = get_callback_fun(),
                .cb_arg = get_callback_arg()
            }
        };

        auto error = rte_epoll_ctl(poll_fd, EPOLL_CTL_ADD,
                                   static_cast<Derived&>(*this).event_fd(),
                                   &m_event);

        if (error) {
            ICP_LOG(ICP_LOG_ERROR, "Could not add poll event for fd %d: %s\n",
                    static_cast<Derived&>(*this).event_fd(),
                    strerror(std::abs(error)));
        }

        return (!error);
    }

    bool del(int poll_fd, void* data)
    {
        assert(m_event.epdata.data == data);
        auto error = rte_epoll_ctl(poll_fd, EPOLL_CTL_DEL,
                                   static_cast<Derived&>(*this).event_fd(),
                                   &m_event);

        if (error) {
            ICP_LOG(ICP_LOG_ERROR, "Could not delete poll event for fd %d: %s\n",
                    static_cast<Derived&>(*this).event_fd(),
                    strerror(std::abs(error)));
        }

        return (!error);
    }

private:
    struct rte_epoll_event m_event;

    /**
     * Use SFINAE to determine whether our derived classes have callback
     * functions and/or arguments.  Use the derived functions if provided,
     * otherwise use nullptr.
     */
    template <typename, typename = std::void_t<> >
    struct has_event_callback_argument : std::false_type{};

    template <typename T>
    struct has_event_callback_argument<T, std::void_t<decltype(std::declval<T>().event_callback_argument())>>
        : std::true_type{};

    void* get_callback_arg()
    {
        if constexpr (has_event_callback_argument<Derived>::value) {
            return (static_cast<Derived&>(*this).event_callback_argument());
        } else {
            return (nullptr);
        }
    }

    template <typename, typename = std::void_t<> >
    struct has_event_callback_function : std::false_type{};

    template <typename T>
    struct has_event_callback_function<T, std::void_t<decltype(std::declval<T>().event_callback_function())>>
        : std::true_type{};

    event_callback get_callback_fun()
    {
        if constexpr (has_event_callback_function<Derived>::value) {
            return (static_cast<Derived&>(*this).event_callback_function());
        } else {
            return (nullptr);
        }
    }
};

}

#endif /* _ICP_PACKETIO_DPDK_POLLABLE_EVENT_TCC_ */
