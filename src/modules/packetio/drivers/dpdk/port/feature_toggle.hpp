#ifndef _OP_PACKETIO_DPDK_PORT_FEATURE_TOGGLE_HPP_
#define _OP_PACKETIO_DPDK_PORT_FEATURE_TOGGLE_HPP_

#include <cstdint>
#include <variant>

namespace openperf::packetio::dpdk::port {

class null_feature
{
public:
    null_feature(uint16_t port_id)
        : m_port(port_id)
    {}

    uint16_t port_id() const { return (m_port); }

    void enable(){};
    void disable(){};

private:
    uint16_t m_port;
};

template <typename Derived, typename... Feature> class feature_toggle
{
public:
    using variant_type = std::variant<Feature...>;

    uint16_t port_id() const
    {
        auto id_visitor = [](auto& s) { return (s.port_id()); };
        return (std::visit(id_visitor, get_feature()));
    }

    void enable()
    {
        if (!m_enabled) {
            auto enable_visitor = [](auto& s) { s.enable(); };
            std::visit(enable_visitor, get_feature());
            m_enabled = true;
        }
    }

    void disable()
    {
        if (m_enabled) {
            auto disable_visitor = [](auto& s) { s.disable(); };
            std::visit(disable_visitor, get_feature());
            m_enabled = false;
        }
    }

protected:
    const variant_type& get_feature() const
    {
        return (static_cast<const Derived&>(*this).feature);
    }

    variant_type& get_feature()
    {
        return (static_cast<Derived&>(*this).feature);
    }

    bool m_enabled = false;
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_FEATURE_TOGGLE_HPP_ */
