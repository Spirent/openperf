#ifndef _OP_DYNAMIC_POOL_HPP_
#define _OP_DYNAMIC_POOL_HPP_

#include "dynamic/api.hpp"
#include "dynamic/threshold.hpp"

#include <chrono>
#include <thread>
#include <vector>
#include <functional>

namespace openperf::dynamic {

using namespace std::chrono_literals;

template <class Stat> class pool
{
public:
    using poll_function = std::function<Stat(void)>;
    using get_function = double (*)(std::string_view, const Stat&);

    using config_t = dynamic::configuration::threshold;
    using config_list = std::vector<config_t>;
    using result_t = dynamic::results::threshold_result;
    using result_list = std::vector<result_t>;
    using duration = std::chrono::milliseconds;

private:
    // Attributes
    bool m_stop = true;
    duration m_interval = 250ms;

    poll_function m_poll;
    get_function m_get;

    result_list m_thresholds;
    std::thread m_thread;
    Stat m_last_stat;

public:
    pool()
        : m_poll(nullptr)
        , m_get(nullptr)
    {}

    pool(poll_function&& pf, get_function&& gf)
        : m_poll(pf)
        , m_get(gf)
    {}

    duration interval() const { return m_interval; }
    void interval(const duration& i) { m_interval = i; }

    void config(const config_list& config)
    {
        m_thresholds.clear();
        std::transform(config.begin(),
                       config.end(),
                       std::back_inserter(m_thresholds),
                       [](auto& i) -> result_t {
                           return {.argument = i.argument,
                                   .id = i.id,
                                   .threshold = {i.value, i.condition}};
                       });
    }

    config_list config() const
    {
        config_list list;
        std::transform(m_thresholds.begin(),
                       m_thresholds.end(),
                       std::back_inserter(list),
                       [](auto& i) -> config_t {
                           return {
                               .argument = i.argument,
                               .id = i.id,
                               .value = i.threshold.value(),
                               .condition = i.threshold.condition(),
                           };
                       });
        return list;
    }

    void reset()
    {
        std::for_each(m_thresholds.begin(), m_thresholds.end(), [](auto& x) {
            x.threshold.reset();
        });
    }

    void stop() { m_stop = true; }

    void start()
    {
        if (!m_stop) return;

        m_thread = std::thread([this]() { loop(); });
    }

    result_list result() const { return m_thresholds; }

private:
    void loop()
    {
        m_stop = false;
        m_last_stat = m_poll();
        while (!m_stop) {
            process(m_poll());
            std::this_thread::sleep_for(m_interval);
        }
    }

    void process(const Stat& stat)
    {
        auto timestamp = std::chrono::system_clock::now();

        for (auto& th : m_thresholds) {
            auto x = m_get(th.argument.x, stat);
            auto y = m_get(th.argument.y, stat);

            auto last_x = m_get(th.argument.x, m_last_stat);
            auto last_y = m_get(th.argument.y, m_last_stat);

            double delta = 0;
            uint64_t weight = 1;
            switch (th.argument.function) {
            case dynamic_argument::DX:
                delta = x - last_x;
                break;
            case dynamic_argument::DXDT: {
                auto delta_x = x - last_x;
                auto delta_t = timestamp - m_last_stat.timestamp;
                delta = (delta_t != 0ns) ? delta_x / delta_t.count() : 0.0;
                break;
            }
            case dynamic_argument::DXDY: {
                auto delta_x = x - last_x;
                auto delta_y = weight = y - last_y;
                delta = (delta_y != 0.0) ? delta_x / delta_y : 0.0;
                break;
            }
            }

            th.threshold.append(delta);
        }

        m_last_stat = stat;
    }
};

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_POOL_HPP_
