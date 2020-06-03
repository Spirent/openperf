#ifndef _OP_PACKETIO_DPDK_PORT_FEATURE_CONTROLLER_HPP_
#define _OP_PACKETIO_DPDK_PORT_FEATURE_CONTROLLER_HPP_

#include "packetio/workers/dpdk/worker_api.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packetio::dpdk {

template <typename... Features> class port_feature_controller
{
public:
    port_feature_controller();

    port_feature_controller(port_feature_controller&&) noexcept;
    port_feature_controller& operator=(port_feature_controller&&) noexcept;

    template <typename T> T& get(size_t);
    template <typename T> const T& get(size_t) const;

    template <typename T> const std::vector<std::unique_ptr<T>>& get() const;

private:
    using container_tuple = std::tuple<std::unique_ptr<Features>...>;
    using feature_container =
        utils::soa_container<std::vector, container_tuple>;
    feature_container m_features;
};

template <typename... Features>
struct sink_feature_controller final : port_feature_controller<Features...>
{
    void update(const worker::fib&, size_t);
};

template <typename... Features>
struct source_feature_controller final : port_feature_controller<Features...>
{
    void update(const worker::tib&, size_t);
};

/**
 * These functions determine if a source/sink needs to enable a specific
 * feature. Clients need to write a template overload to handle each
 * specific feature.
 */
template <typename Feature>
bool need_sink_feature(const worker::fib&, size_t, const Feature&);

template <typename Feature>
bool need_source_feature(const worker::tib&, size_t, const Feature&);

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_PORT_FEATURE_CONTROLLER_HPP_ */
