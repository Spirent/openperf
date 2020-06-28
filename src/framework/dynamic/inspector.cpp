#include "inspector.hpp"

#include <set>
#include <functional>

namespace openperf::dynamic {

inspector::inspector()
    : inspector(nullptr)
{}

inspector::inspector(poll_function&& function)
    : m_stop(true)
    , m_poll(function)
{}

void inspector::configure(const config_list& config)
{
    std::set<std::string> ids;

    m_thresholds.clear();
    for (const auto& item : config) {
        auto id = item.id;
        if (id.empty()) id = core::to_string(core::uuid::random());
        if (auto check = config::op_config_validate_id_string(id); !check)
            throw std::domain_error(check.error().c_str());

        // if (ids.containts(id)) // C++20
        if (ids.count(id))
            throw std::invalid_argument("Dynamic Result with ID '" + id
                                        + "' already exists.");

        ids.insert(id);
        m_thresholds.push_back({.argument = item.argument,
                                .id = id,
                                .threshold = {item.value, item.condition}});
    }
}

inspector::config_list inspector::config() const
{
    config_list list;
    std::transform(m_thresholds.begin(),
                   m_thresholds.end(),
                   std::back_inserter(list),
                   [](const auto& i) -> config_t {
                       return {
                           .argument = i.argument,
                           .id = i.id,
                           .value = i.threshold.value(),
                           .condition = i.threshold.condition(),
                       };
                   });
    return list;
}

void inspector::reset()
{
    std::for_each(m_thresholds.begin(), m_thresholds.end(), [](auto& x) {
        x.threshold.reset();
    });
}

void inspector::start(const config_list& config)
{
    if (!m_stop) return;

    configure(config);
    start();
}

void inspector::start()
{
    if (!m_stop) return;
    if (m_thresholds.empty()) return;

    m_thread = std::thread([this]() { loop(); });
}

void inspector::loop()
{
    using chronometer = std::chrono::system_clock;

    m_stop = false;
    m_last_stat = m_poll();
    while (!m_stop) {
        auto time_start = chronometer::now();
        spin();
        auto spin_time = chronometer::now() - time_start;
        std::this_thread::sleep_for(m_interval - spin_time);
    }
}

void inspector::spin()
{
    auto stat = m_poll();
    for (auto& th : m_thresholds)
        th.threshold.append(delta(th.argument, *stat));

    m_last_stat = std::move(stat);
}

int inspector::weight(const dynamic_argument& arg, const inspectable& stat)
{
    uint64_t weight = 1;
    switch (arg.function) {
    case dynamic_argument::DXDY: {
        auto y = stat.field(arg.y);
        auto last_y = m_last_stat->field(arg.y);
        weight = y - last_y;
        break;
    }
    default:
        break;
    }

    return weight;
}

double inspector::delta(const dynamic_argument& arg, const inspectable& stat)
{
    auto x = stat.field(arg.x);
    auto last_x = m_last_stat->field(arg.x);
    auto delta_x = x - last_x;

    double delta = 0;
    switch (arg.function) {
    case dynamic_argument::DX: {
        delta = delta_x;
        break;
    }
    case dynamic_argument::DXDT: {
        auto t = stat.timestamp();
        auto last_t = m_last_stat->timestamp();
        auto delta_t = t - last_t;
        delta = (delta_t != 0ns) ? delta_x / delta_t.count() : 0.0;
        break;
    }
    case dynamic_argument::DXDY: {
        auto y = stat.field(arg.y);
        auto last_y = m_last_stat->field(arg.y);
        auto delta_y = y - last_y;
        delta = (delta_y != 0.0) ? delta_x / delta_y : 0.0;
        break;
    }
    }

    return delta;
}

} // namespace openperf::dynamic
