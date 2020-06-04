#ifndef _OP_PACKETIO_DPDK_PORT_CALLBACK_HPP_
#define _OP_PACKETIO_DPDK_PORT_CALLBACK_HPP_

#include <algorithm>
#include <map>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/model/port_info.hpp"

namespace openperf::packetio::dpdk::port {

template <typename Derived> class callback
{
public:
    callback(uint16_t port_id)
        : m_port(port_id)
    {}

    virtual ~callback()
    {
        if (callback_map().empty()) { return; }

        auto& child = static_cast<Derived&>(*this);
        child.disable();
    }

    callback(callback&& other) noexcept
        : m_callbacks(std::move(other.m_callbacks))
        , m_port(other.m_port)
    {}

    callback& operator=(callback&& other) noexcept
    {
        if (this != &other) {
            m_callbacks = std::move(other.m_callbacks);
            m_port = other.m_port;
        }
        return (*this);
    }

    uint16_t port_id() const { return (m_port); };

    using callback_map_type = std::map<uint16_t, const rte_eth_rxtx_callback*>;

protected:
    callback_map_type& callback_map() { return (m_callbacks); };

private:
    callback_map_type m_callbacks;
    uint16_t m_port;
};

namespace detail {
template <typename, typename = std::void_t<>>
struct has_callback_argument : std::false_type
{};

template <typename T>
struct has_callback_argument<
    T,
    std::void_t<decltype(std::declval<T>().callback_arg())>> : std::true_type
{};

template <typename T> void* get_callback_argument(const T& thing)
{
    if constexpr (has_callback_argument<T>::value) {
        return (thing.callback_arg());
    } else {
        return (nullptr);
    }
}

} // namespace detail
template <typename Derived>
struct rx_callback : public callback<rx_callback<Derived>>
{
    using callback<rx_callback<Derived>>::callback;
    using rx_callback_fn = uint16_t (*)(uint16_t port_id,
                                        uint16_t queue_id,
                                        rte_mbuf* packets[],
                                        uint16_t nb_packets,
                                        uint16_t max_packets,
                                        void* user_param);
    void enable()
    {
        auto& child = static_cast<Derived&>(*this);
        OP_LOG(OP_LOG_INFO,
               "Enabling software %s on port %u\n",
               child.description().c_str(),
               this->port_id());

        auto arg = detail::get_callback_argument(child);
        auto& map = this->callback_map();
        const auto q_count = model::port_info(this->port_id()).rx_queue_count();

        for (uint16_t q = 0; q < q_count; q++) {
            auto cb = rte_eth_add_rx_callback(
                this->port_id(), q, child.callback(), arg);
            if (!cb) {
                throw std::runtime_error("Could not add " + child.description()
                                         + " callback to port "
                                         + std::to_string(this->port_id())
                                         + ", queue " + std::to_string(q));
            }
            map.emplace(q, cb);
        }
    }

    void disable()
    {
        auto& child = static_cast<Derived&>(*this);
        OP_LOG(OP_LOG_INFO,
               "Disabling software %s on port %u\n",
               child.description().c_str(),
               this->port_id());

        auto& map = this->callback_map();
        std::for_each(std::begin(map), std::end(map), [&](auto&& item) {
            rte_eth_remove_rx_callback(
                this->port_id(), item.first, item.second);
        });

        map.clear();
    }
};

template <typename Derived>
struct tx_callback : public callback<tx_callback<Derived>>
{
    using callback<tx_callback<Derived>>::callback;
    using tx_callback_fn = uint16_t (*)(uint16_t port_id,
                                        uint16_t queue_id,
                                        rte_mbuf* packets[],
                                        uint16_t nb_packets,
                                        void* user_param);
    void enable()
    {
        auto& child = static_cast<Derived&>(*this);
        OP_LOG(OP_LOG_INFO,
               "Enabling software %s on port %u\n",
               child.description().c_str(),
               this->port_id());

        auto arg = detail::get_callback_argument(child);
        auto& map = this->callback_map();
        const auto q_count = model::port_info(this->port_id()).tx_queue_count();

        for (uint16_t q = 0; q < q_count; q++) {
            auto cb = rte_eth_add_tx_callback(
                this->port_id(), q, child.callback(), arg);
            if (!cb) {
                throw std::runtime_error("Could not add " + child.description()
                                         + " callback to port "
                                         + std::to_string(this->port_id())
                                         + ", queue " + std::to_string(q));
            }
            map.emplace(q, cb);
        }
    }

    void disable()
    {
        auto& child = static_cast<Derived&>(*this);
        OP_LOG(OP_LOG_INFO,
               "Disabling software %s on port %u\n",
               child.description().c_str(),
               this->port_id());

        auto& map = this->callback_map();
        std::for_each(std::begin(map), std::end(map), [&](auto&& item) {
            rte_eth_remove_tx_callback(
                this->port_id(), item.first, item.second);
        });

        map.clear();
    }
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_CALLBACK_TCC_ */
