#ifndef _PACKET_COUNTER_H_
#define _PACKET_COUNTER_H_

#include <atomic>
#include <string>

template <typename T> struct packet_counter
{
    const std::string name_;
    const unsigned limit_;
    std::atomic<T> pkts_;
    std::atomic<T> octets_total_;
    std::atomic<T> octets_avg_;
    std::atomic<T> octets_min_;
    std::atomic<T> octets_max_;

    packet_counter(std::string_view name)
        : name_(name)
        , limit_(std::numeric_limits<unsigned>::max())
        , pkts_(0)
        , octets_total_(0)
        , octets_avg_(0)
        , octets_min_(std::numeric_limits<T>::max())
        , octets_max_(0)
    {}
    ~packet_counter() = default;

    void reset()
    {
        pkts_.store(0, std::memory_order_relaxed);
        octets_total_.store(0, std::memory_order_relaxed);
        octets_avg_.store(0, std::memory_order_relaxed);
        octets_min_.store(std::numeric_limits<T>::max(),
                          std::memory_order_relaxed);
        octets_max_.store(0, std::memory_order_relaxed);
    }

    void add(T octets)
    {
        auto last_pkts = pkts_.fetch_add(1, std::memory_order_relaxed);
        octets_total_.fetch_add(octets, std::memory_order_relaxed);

        auto avg = octets_avg_.load(std::memory_order_relaxed);
        auto adjust =
            (octets > avg ? octets - avg : 0)
            / std::max(1UL, std::min(static_cast<T>(limit_), last_pkts + 1));
        octets_avg_.fetch_add(adjust, std::memory_order_relaxed);

        octets_min_.store(
            std::min(octets, octets_min_.load(std::memory_order_relaxed)),
            std::memory_order_relaxed);
        octets_max_.store(
            std::max(octets, octets_max_.load(std::memory_order_relaxed)),
            std::memory_order_relaxed);
    }
};

#endif /* _PACKET_COUNTER_H_ */
