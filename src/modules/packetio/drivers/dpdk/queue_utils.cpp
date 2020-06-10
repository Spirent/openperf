#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <vector>

#include "packetio/drivers/dpdk/arg_parser.hpp"
#include "packetio/drivers/dpdk/model/physical_port.hpp"
#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/drivers/dpdk/queue_utils.hpp"

namespace openperf::packetio::dpdk::queue {

struct queue_info
{
    uint16_t count; /**< The number of queues in use */
    uint16_t max; /**< The maximum number of extra queues that could be used */
    uint16_t avail;       /**< The number of queues available to use */
    uint16_t distributed; /**< The number of distributions that have occurred;
                               keeping state here is ugly, but convenient */
};

struct port_queue_info
{
    struct queue_info rxq; /**< rx queue info */
    struct queue_info txq; /**< tx queue info */
    uint16_t count; /**< the number of ports that can share extra queues */
};

struct worker_config
{
    unsigned load;   /**< worker load in Mbps */
    unsigned queues; /**< number of queues serviced by the worker */
};

/**
 * This looks complicated, but really we just try to assign a reasonable number
 * of queues based on our port/queue/thread constraints.
 */
static uint16_t suggested_queue_count(uint16_t nb_ports,
                                      uint16_t nb_queues,
                                      uint16_t nb_threads)
{
    return (
        nb_threads == 1
            ? 1
            : nb_ports < nb_threads
                  ? (nb_threads % nb_ports == 0
                         ? std::min(
                             nb_queues,
                             static_cast<uint16_t>(nb_threads / nb_ports))
                         : std::min(nb_queues, nb_threads))
                  : std::min(nb_queues, static_cast<uint16_t>(nb_threads - 1)));
}

/**
 * Extra queue distribution algorithm
 * TL;DR: If we have free workers, give extra queues to the fastest ports.
 */
static std::map<int, port_queue_info>
bonus_queue_info(const std::vector<model::port_info>& port_info,
                 uint16_t rx_extras,
                 uint16_t tx_extras)
{
    std::map<int, port_queue_info> queue_info;
    for (auto& info : port_info) {
        if (queue_info.find(info.max_speed()) == queue_info.end()) {
            queue_info.emplace(info.max_speed(), port_queue_info());
        }

        auto& qinfo = queue_info.at(info.max_speed());

        qinfo.count++;
        qinfo.rxq.count += info.rx_queue_default();
        qinfo.rxq.max += info.rx_queue_max();
        qinfo.txq.count += info.tx_queue_default();
        qinfo.txq.max += info.tx_queue_max();
    }

    /*
     * Now distribute extra queues to the fastest ports, hence we reverse the
     * normal loop direction.
     */
    for (auto it = std::rbegin(queue_info); it != std::rend(queue_info); ++it) {
        uint16_t to_give = std::min(
            rx_extras,
            static_cast<uint16_t>(it->second.rxq.max - it->second.rxq.count));
        it->second.rxq.avail += to_give;
        rx_extras -= to_give;

        if (!rx_extras) break;
    }

    for (auto it = std::rbegin(queue_info); it != std::rend(queue_info); ++it) {
        uint16_t to_give = std::min(
            tx_extras,
            static_cast<uint16_t>(it->second.txq.max - it->second.txq.count));
        it->second.txq.avail += to_give;
        tx_extras -= to_give;

        if (!tx_extras) break;
    }

    return (queue_info);
}

/**
 * Round x up to the nearest multiple of 'multiple'.
 */
template <typename T> static __attribute__((const)) T round_up(T x, T multiple)
{
    assert(multiple); /* don't want a multiple of 0 */
    auto remainder = x % multiple;
    return (remainder == 0 ? x : x + multiple - remainder);
}

/**
 * Get the count of items in bucket 'n' if we evenly distribute a 'total' count
 * across the specified number of 'buckets'.
 * Obviously, 0 <= n < buckets.
 */
template <typename T, typename U, typename V>
static __attribute__((const)) T distribute(T total, U buckets, V n)
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_integral_v<U>);
    static_assert(std::is_integral_v<V>);

    assert(n < buckets);
    auto base = total / buckets;
    return (static_cast<T>(n < total % buckets ? base + 1 : base));
}

uint16_t offset_bit(const model::core_mask& mask, uint16_t offset)
{
    assert(mask.count());

    offset %= mask.count();

    auto idx = 0U;
    while (idx < mask.size()) {
        if (mask[idx]) {
            if (!offset) { break; }
            offset--;
        }
        idx++;
    }

    return (idx);
}

uint16_t worker_id(const std::vector<worker_config>& workers,
                   const model::core_mask& worker_mask,
                   uint16_t offset)
{
    assert(workers.size() < std::numeric_limits<uint16_t>::max());

    /* First, see if we can find a worker with less queues than the others */
    const uint16_t bit_offset = offset_bit(worker_mask, offset);
    bool have_candidate = false;
    uint16_t to_return = bit_offset;
    for (size_t i = 0; i < workers.size(); i++) {
        auto id = (bit_offset + i) % workers.size();
        if (!worker_mask[id]) { continue; }
        if (workers[id].queues < workers[to_return].queues) {
            have_candidate = true;
            to_return = id;
        }
    }

    if (!have_candidate) {
        /*
         * No obvious candidate based on queue counts; look for a worker with
         * less load. Otherwise, just use the offset.
         */
        for (size_t i = 0; i < workers.size(); i++) {
            auto id = (bit_offset + i) % workers.size();
            if (!worker_mask[id]) { continue; }
            if (workers[id].load < workers[to_return].load) { to_return = id; }
        }
    }

    return (to_return);
}

static std::vector<descriptor>
distribute_queues(const std::vector<model::port_info>& port_info,
                  uint16_t worker_id)
{
    std::vector<descriptor> descriptors;
    descriptors.reserve(port_info.size());

    /* Simple case; one worker handling one queue per port */
    for (auto& info : port_info) {
        descriptors.emplace_back(descriptor{.worker_id = worker_id,
                                            .port_id = info.id(),
                                            .queue_id = 0,
                                            .mode = queue_mode::RXTX});
    }

    return (descriptors);
}

static std::vector<descriptor>
distribute_queues(const std::vector<model::port_info>& port_info,
                  const model::core_mask& worker_mask,
                  queue_mode mode)
{
    assert(mode == queue_mode::RX || mode == queue_mode::TX);
    assert(worker_mask.any());

    /*
     * Calculate the minimum number of queues we need based on our
     * suggested queue count.
     */
    auto q_min = std::accumulate(
        std::begin(port_info),
        std::end(port_info),
        0UL,
        [&](int x, const model::port_info& pi) {
            return (x
                    + suggested_queue_count(port_info.size(),
                                            mode == queue_mode::RX
                                                ? pi.rx_queue_default()
                                                : pi.tx_queue_default(),
                                            worker_mask.count()));
        });

    /*
     * Compare this count to the number of workers we have to
     * generate a count of 'extra' queues we could use if we have
     * available, unused workers.
     */
    uint16_t q_extra = round_up(q_min, worker_mask.count()) - q_min;

    /* Figure out if any ports can use these 'bonus' queues */
    auto queue_bonus = mode == queue_mode::RX
                           ? bonus_queue_info(port_info, q_extra, 0)
                           : bonus_queue_info(port_info, 0, q_extra);

    /*
     * Generate some record keeping structs. These contain per worker
     * queue and load data based on our assignments to workers.
     */
    auto worker_configs = std::vector<worker_config>(worker_mask.size());

    /* This is the actual configuration that we will return */
    auto descriptors = std::vector<descriptor>{};

    /*
     * Begin loop proper; assign queues to available workers as fairly
     * as possible.
     */
    for (const auto& info : port_info) {
        auto& bonus = queue_bonus[info.max_speed()];
        /* Only hand out bonus queues if all ports can get one */
        auto& bonus_info = mode == queue_mode::RX ? bonus.rxq : bonus.txq;
        const auto q_bonus =
            bonus_info.avail > bonus.count ? distribute(
                bonus_info.avail, bonus_info.count, bonus_info.distributed++)
                                           : 0;
        auto q_count = suggested_queue_count(port_info.size(),
                                             mode == queue_mode::RX
                                                 ? info.rx_queue_default()
                                                 : info.tx_queue_default(),
                                             worker_mask.count())
                       + q_bonus;

        for (auto q = 0; q < q_count; q++) {
            descriptors.emplace_back(descriptor{
                .worker_id = worker_id(worker_configs, worker_mask, info.id()),
                .port_id = info.id(),
                .queue_id = static_cast<uint16_t>(q),
                .mode = mode});

            const auto& last = descriptors.back();
            worker_configs[last.worker_id].load += info.max_speed() / q_count;
            worker_configs[last.worker_id].queues++;
        }
    }

    return (descriptors);
}

/* Compress any numeric value into a non-zero uint16_t */
template <typename T> uint16_t to_u16(T x)
{
    return (static_cast<uint16_t>(
        std::clamp(x, static_cast<T>(1), static_cast<T>(0xffff))));
}

static std::pair<model::core_mask, model::core_mask>
unique_masks(const model::core_mask& mask)
{
    /* Check for explicit tx/rx masks that provide cores for the given mask */
    auto rx_mask = config::rx_core_mask().value_or(model::core_mask{}) & mask;
    auto tx_mask = config::tx_core_mask().value_or(model::core_mask{}) & mask;

    /* Figure out which cores still need to be assigned */
    const auto available = mask & (~rx_mask) & (~tx_mask);

    /*
     * If only one core is available in total, then we need to use it for
     * both rx and tx.
     */
    if (rx_mask.none() && tx_mask.none() && available.count() == 1) {
        return {available, available};
    }

    /* Else, distribute the available cores to rx/tx usage */
    const auto to_rx = distribute(available.count(), 2U, 0U);
    auto distributed = 0U;
    for (size_t i = 0; i < available.size(); i++) {
        if (!available[i]) { continue; }
        if (distributed < to_rx) {
            rx_mask.set(i);
        } else {
            tx_mask.set(i);
        }
        distributed++;
    }

    return {rx_mask, tx_mask};
}

std::vector<descriptor>
distribute_queues(const std::vector<model::port_info>& port_info,
                  const model::core_mask& mask)
{
    /* With only 1 worker, no distribution is necessary */
    if (mask.count() == 1) {
        /* Find the flipped bit, aka the core id */
        auto idx = 0U;
        while (idx < mask.size() && !mask[idx]) { ++idx; }

        return (distribute_queues(port_info, idx));
    }

    const auto [rx_mask, tx_mask] = unique_masks(mask);

    auto queues = distribute_queues(port_info, rx_mask, queue_mode::RX);
    const auto tx_queues =
        distribute_queues(port_info, tx_mask, queue_mode::TX);
    queues.insert(std::end(queues), std::begin(tx_queues), std::end(tx_queues));

    return (queues);
}

std::map<int, count>
get_port_queue_counts(const std::vector<queue::descriptor>& descriptors)
{
    std::map<int, count> port_queue_counts;

    for (auto& d : descriptors) {
        if (port_queue_counts.find(d.port_id) == port_queue_counts.end()) {
            port_queue_counts.emplace(d.port_id, count{});
        }

        auto& queue_count = port_queue_counts[d.port_id];

        switch (d.mode) {
        case queue::queue_mode::RX:
            queue_count.rx++;
            break;
        case queue::queue_mode::TX:
            queue_count.tx++;
            break;
        case queue::queue_mode::RXTX:
            queue_count.tx++;
            queue_count.rx++;
            break;
        default:
            break;
        }
    }

    return (port_queue_counts);
}

} // namespace openperf::packetio::dpdk::queue
