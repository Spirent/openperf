#include <cassert>

#include "packetio/drivers/dpdk/topology_utils.hpp"
#include "packetio/drivers/dpdk/secondary/arg_parser.hpp"

namespace openperf::packetio::dpdk::topology {

std::vector<uint16_t> get_ports()
{
    auto ids = std::vector<uint16_t>{};
    uint16_t port_id = 0;
    RTE_ETH_FOREACH_DEV (port_id) {
        ids.emplace_back(port_id);
    }

    return (ids);
}

std::vector<uint16_t> get_tx_queues(uint16_t port_id)
{
    const auto tx_port_queues = config::tx_port_queues();
    return (tx_port_queues.count(port_id) ? tx_port_queues.at(port_id)
                                          : std::vector<uint16_t>{});
}

static std::vector<queue::descriptor>
translate_port_queue_ids(const std::vector<queue::descriptor>& input)
{
    const auto tx_port_queues = config::tx_port_queues();

    const auto contains_queue = [](const auto& q_map, uint16_t p, uint16_t q) {
        return (q_map.size() > p && q_map.at(p).size() > q);
    };

    auto tmp = std::vector<queue::descriptor>{};
    std::copy_if(
        std::begin(input),
        std::end(input),
        std::back_inserter(tmp),
        [&](const auto& d) {
            assert(d.direction != queue::queue_direction::NONE);

            return (d.direction == queue::queue_direction::RX
                    || contains_queue(tx_port_queues, d.port_id, d.queue_id));
        });

    auto output = std::vector<queue::descriptor>{};
    std::transform(
        std::begin(tmp),
        std::end(tmp),
        std::back_inserter(output),
        [&](const auto& d) {
            assert(d.direction != queue::queue_direction::NONE);

            return (queue::descriptor{
                .worker_id = d.worker_id,
                .port_id = d.port_id,
                .queue_id = (d.direction == queue::queue_direction::RX
                                 ? d.queue_id
                                 : tx_port_queues.at(d.port_id)[d.queue_id]),
                .direction = d.direction});
        });

    return (output);
}

std::vector<queue::descriptor>
queue_distribute(const std::vector<uint16_t>& port_ids)
{
    return (translate_port_queue_ids(index_queue_distribute(port_ids)));
}

} // namespace openperf::packetio::dpdk::topology
