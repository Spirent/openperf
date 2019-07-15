#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <vector>

#include "packetio/drivers/dpdk/model/physical_port.h"
#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/drivers/dpdk/queue_utils.h"

namespace icp::packetio::dpdk::queue {

struct queue_info {
    uint16_t count;        /**< The number of queues in use */
    uint16_t max;          /**< The maximum number of extra queues that could be used */
    uint16_t avail;        /**< The number of queues available to use */
    uint16_t distributed;  /**< The number of distributions that have occurred;
                                keeping state here is ugly, but convenient */
};

struct port_queue_info {
    struct queue_info rxq;  /**< rx queue info */
    struct queue_info txq;  /**< tx queue info */
    uint16_t count;         /**< the number of ports that can share extra queues */
};

struct worker_config {
    unsigned load;    /**< worker load in Mbps */
    unsigned queues;  /**< number of queues serviced by the worker */
};

/**
 * This looks complicated, but really we just try to assign a reasonable number of
 * queues based on our port/queue/thread constraints.
 */
static uint16_t suggested_queue_count(uint16_t nb_ports,
                                      uint16_t nb_queues,
                                      uint16_t nb_threads)
{
    return (nb_threads == 1 ? 1
            : nb_ports < nb_threads ? (nb_threads % nb_ports == 0
                                       ? std::min(nb_queues,
                                                  static_cast<uint16_t>(nb_threads / nb_ports))
                                       : std::min(nb_queues, nb_threads))
            : std::min(nb_queues, static_cast<uint16_t>(nb_threads - 1)));
}

/**
 * Extra queue distribution algorithm
 * TL;DR: If we have free workers, give extra queues to the fastest ports.
 */
static std::map<int, port_queue_info> bonus_queue_info(const std::vector<model::port_info>& port_info,
                                                       uint16_t rx_extras, uint16_t tx_extras)
{
    std::map<int, port_queue_info> queue_info;
    for (auto& info : port_info) {
        if (queue_info.find(info.max_speed()) == queue_info.end()) {
            queue_info.emplace(info.max_speed(), port_queue_info());
        }

        auto& qinfo = queue_info.at(info.max_speed());

        qinfo.count++;
        qinfo.rxq.count += info.rx_queue_default();
        qinfo.rxq.max   += info.rx_queue_max();
        qinfo.txq.count += info.tx_queue_default();
        qinfo.txq.max   += info.tx_queue_max();
    }

    /*
     * Now distribute extra queues to the fastest ports, hence we reverse the normal
     * loop direction.
     */
    for (auto it = std::rbegin(queue_info); it != std::rend(queue_info); ++it) {
        uint16_t to_give = std::min(rx_extras,
                                    static_cast<uint16_t>(it->second.rxq.max - it->second.rxq.count));
        it->second.rxq.avail += to_give;
        rx_extras -= to_give;

        if (!rx_extras) break;
    }

    for (auto it = std::rbegin(queue_info); it != std::rend(queue_info); ++it) {
        uint16_t to_give = std::min(tx_extras,
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
template <typename T>
static __attribute__((const)) T round_up(T x, T multiple)
{
    assert(multiple);  /* don't want a multiple of 0 */
    auto remainder = x % multiple;
    return (remainder == 0 ? x : x + multiple - remainder);
}

/**
 * Get the count of items in bucket 'n' if we evenly distribute a 'total' count
 * across the specified number of 'buckets'.
 * Obviously, 0 <= n < buckets.
 */
template <typename T>
static __attribute__((const)) T distribute(T total, T buckets, T n)
{
    assert(n < buckets);
    auto base = total / buckets;
    return (n < total % buckets ? base + 1 : base);
}

uint16_t worker_id(const std::vector<worker_config>& workers, uint16_t offset)
{
    /* First, see if we can find a worker with less queues than the others */
    bool have_candidate = false;
    uint16_t to_return = offset % workers.size();
    for (uint16_t i = 0; i < workers.size(); i++) {
        auto id = (offset + i) % workers.size();
        if (workers[id].queues < workers[to_return].queues) {
            have_candidate = true;
            to_return = id;
        }
    }

    if (!have_candidate) {
        /*
         * No obvious candidate based on queue counts; look for a worker with
         * less load.  Otherwise, just use the offset.
         */
        for (uint16_t i = 0; i < workers.size(); i++) {
            auto id = (offset + i) % workers.size();
            if (workers[id].load < workers[to_return].load) {
                to_return = id;
            }
        }
    }

    return (to_return);
}

static std::vector<descriptor> distribute_queues(const std::vector<model::port_info>& port_info)
{
    std::vector<descriptor> descriptors;

    /* Simple case; one worker handling one queue per port */
    for (auto& info : port_info) {
        descriptors.emplace_back(
            descriptor {
                .worker_id = 0,
                .port_id = info.id(),
                .queue_id = 0,
                .mode = queue_mode::RXTX });
    }

    return (descriptors);
}

static std::vector<descriptor> distribute_queues(const std::vector<model::port_info>& port_info,
                                                 uint16_t rx_workers, uint16_t tx_workers)
{
    /*
     * Calculate the minimum number of {rx,tx} queues we need based on our suggested
     * queue count.
     */
    uint16_t rxq_min = std::accumulate(std::begin(port_info), std::end(port_info), 0,
                                       [&](int x, const model::port_info& pi) {
                                           return (x + suggested_queue_count(port_info.size(),
                                                                             pi.rx_queue_default(),
                                                                             rx_workers));
                                       });
    uint16_t txq_min = std::accumulate(std::begin(port_info), std::end(port_info), 0,
                                       [&](int x, const model::port_info& pi) {
                                           return (x + suggested_queue_count(port_info.size(),
                                                                             pi.tx_queue_default(),
                                                                             tx_workers));
                                       });

    /*
     * Compare this count to the number of {rx,tx} workers we have to
     * generate a count of 'extra' queues we could use if we have available,
     * unused workers.
     */
    uint16_t rxq_extra = round_up(rxq_min, rx_workers) - rxq_min;
    uint16_t txq_extra = round_up(txq_min, tx_workers) - txq_min;

    /* Figure out if any ports can use these 'bonus' queues */
    auto queue_bonus = bonus_queue_info(port_info, rxq_extra, txq_extra);

    /*
     * Generate some record keeping structs.  These contain per worker
     * queue and load data based on our assignments to workers.
     */
    std::vector<worker_config> rx_workers_config, tx_workers_config;
    for (uint16_t i = 0; i < rx_workers; i++) {
        rx_workers_config.emplace_back(worker_config());
    }
    for (uint16_t i = 0; i < tx_workers; i++) {
        tx_workers_config.emplace_back(worker_config());
    }

    /* This is the actual configuration that we will return */
    std::vector<descriptor> descriptors;

    /*
     * Begin loop proper; assign {tx, rx} queues to available workers as fairly as
     * possible.
     */
    for (auto& info : port_info) {
        auto& bonus = queue_bonus[info.max_speed()];
        /* Only hand out bonus queues if all ports of the same speed can get one */
        auto rx_bonus = (bonus.rxq.avail >= bonus.count
                         ? distribute(bonus.rxq.avail,
                                      bonus.rxq.count,
                                      bonus.rxq.distributed++)
                         : 0);
        uint16_t rxq_count = (suggested_queue_count(port_info.size(),
                                                info.rx_queue_default(),
                                                rx_workers)
                              + rx_bonus);

        /* And assign them to workers */
        for (uint16_t q = 0; q < rxq_count; q++) {
            descriptors.emplace_back(
                descriptor{
                    .worker_id   = worker_id(rx_workers_config, info.id()),
                    .port_id     = info.id(),
                    .queue_id    = q,
                    .mode        = queue_mode::RX });

            auto& last = descriptors.back();
            rx_workers_config[last.worker_id].load += info.max_speed() / rxq_count;
            rx_workers_config[last.worker_id].queues++;
        }

        /* And we do the same for tx */
        auto tx_bonus = (bonus.txq.avail >= bonus.count
                         ? distribute(bonus.txq.avail,
                                      bonus.txq.count,
                                      bonus.txq.distributed++)
                         : 0);

        uint16_t txq_count = (suggested_queue_count(port_info.size(),
                                                    info.tx_queue_default(),
                                                    tx_workers)
                              + tx_bonus);

        for (uint16_t q = 0; q < txq_count; q++) {
            descriptors.emplace_back(
                descriptor{
                    .worker_id   = worker_id(tx_workers_config, info.id()),
                    .port_id     = info.id(),
                    .queue_id    = q,
                    .mode        = queue_mode::TX });

            auto& last = descriptors.back();
            tx_workers_config[last.worker_id].load += info.max_speed() / txq_count;
            tx_workers_config[last.worker_id].queues++;
            /*
             * Offset TX workers by RX workers.  It's easier to it here than
             * to add (more) complexity to the distribution functions.
             */
            last.worker_id += rx_workers;
        }
    }

    return (descriptors);
}

std::vector<descriptor> distribute_queues(const std::vector<model::port_info>& port_info, uint16_t q_workers)
{
    return (q_workers == 1
            ? distribute_queues(port_info)
            : distribute_queues(port_info,
                                distribute(q_workers,
                                           static_cast<uint16_t>(2),
                                           static_cast<uint16_t>(0)),
                                distribute(q_workers,
                                           static_cast<uint16_t>(2),
                                           static_cast<uint16_t>(1))));
}

}
