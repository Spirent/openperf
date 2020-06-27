#include "poller.hpp"

#include <set>
#include <functional>

namespace openperf::dynamic {

// Constructors & Destructor
poller::poller()
    : poller(nullptr)
{}

poller::poller(poll_function&& function)
    : m_stop(true)
    , m_poll(function)
{}

// Methods : public
void poller::configure(const dynamic::configuration& config)
{
    std::set<std::string> ids;
    auto get_id = [&ids](const std::string& s) {
        auto id = s;
        if (id.empty()) id = core::to_string(core::uuid::random());
        if (auto check = config::op_config_validate_id_string(id); !check)
            throw std::domain_error(check.error().c_str());

        // if (ids.containts(id)) // C++20
        if (ids.count(id))
            throw std::invalid_argument("Item with ID '" + id
                                        + "' already exists.");

        ids.insert(id);
        return id;
    };

    m_thresholds.clear();
    for (const auto& item : config.thresholds) {
        argument_check(item.argument);

        m_thresholds.push_back({.argument = item.argument,
                                .id = get_id(item.id),
                                .threshold = {item.value, item.condition}});
    }

    ids.clear();
    m_tdigests.clear();
    for (const auto& item : config.tdigests) {
        argument_check(item.argument);

        auto result = dynamic::results::tdigest{
            .argument = item.argument,
            .id = get_id(item.id),
            .compression = item.compression,
            .tdigest = std::make_shared<dynamic::tdigest>(item.compression),
        };

        m_tdigests.push_back(result);
    }
}

dynamic::configuration poller::config() const
{
    dynamic::configuration conf;
    std::transform(m_thresholds.begin(),
                   m_thresholds.end(),
                   std::back_inserter(conf.thresholds),
                   [](const auto& i) -> configuration::threshold {
                       return {
                           .argument = i.argument,
                           .id = i.id,
                           .value = i.threshold.value(),
                           .condition = i.threshold.condition(),
                       };
                   });

    std::transform(m_tdigests.begin(),
                   m_tdigests.end(),
                   std::back_inserter(conf.tdigests),
                   [](const auto& i) -> configuration::tdigest {
                       return {
                           .argument = i.argument,
                           .id = i.id,
                           .compression = i.compression,
                       };
                   });
    return conf;
}

void poller::reset()
{
    std::for_each(m_thresholds.begin(), m_thresholds.end(), [](auto& x) {
        x.threshold.reset();
    });

    std::for_each(m_tdigests.begin(), m_tdigests.end(), [](auto& x) {
        x.tdigest.reset();
    });
}

void poller::start(const dynamic::configuration& config)
{
    if (!m_stop) return;

    configure(config);
    start();
}

void poller::start()
{
    if (!m_stop) return;
    if (m_thresholds.empty() && m_tdigests.empty()) return;

    m_thread = std::thread([this]() { loop(); });
}

void poller::stop()
{
    if (m_stop) return;
    m_stop = true;
    m_thread.join();
}

dynamic::results poller::result() const
{
    return dynamic::results{.thresholds = m_thresholds, .tdigests = m_tdigests};
}

// Methods : private

void poller::loop()
{
    using chronometer = openperf::timesync::chrono::monotime;

    m_stop = false;
    m_last_stat = m_poll();
    while (!m_stop) {
        auto time_start = chronometer::now();
        spin();
        auto spin_time = chronometer::now() - time_start;
        std::this_thread::sleep_for(m_interval - spin_time);
    }
}

void poller::spin()
{
    auto stat = m_poll();
    for (auto& th : m_thresholds)
        th.threshold.append(delta(th.argument, *stat));

    for (auto& td : m_tdigests)
        td.tdigest->insert(delta(td.argument, *stat),
                           weight(td.argument, *stat));

    m_last_stat = std::move(stat);
}

int poller::weight(const dynamic::argument_t& arg, const pollable& stat)
{
    int weight = 1;
    switch (arg.function) {
    case dynamic::argument_t::DXDY: {
        auto y = stat.field(arg.y);
        auto last_y = m_last_stat->field(arg.y);
        weight = static_cast<int>(y - last_y);
        break;
    }
    default:
        break;
    }

    return weight;
}

double poller::delta(const dynamic::argument_t& arg, const pollable& stat)
{
    auto x = stat.field(arg.x);
    auto last_x = m_last_stat->field(arg.x);
    auto delta_x = x - last_x;

    double delta = 0;
    switch (arg.function) {
    case dynamic::argument_t::DX: {
        delta = delta_x;
        break;
    }
    case dynamic::argument_t::DXDT: {
        auto t = stat.timestamp();
        auto last_t = m_last_stat->timestamp();
        auto delta_t = t - last_t;
        delta = (delta_t != 0ns) ? delta_x / delta_t.count() : 0.0;
        break;
    }
    case dynamic::argument_t::DXDY: {
        auto y = stat.field(arg.y);
        auto last_y = m_last_stat->field(arg.y);
        auto delta_y = y - last_y;
        delta = (delta_y != 0.0) ? delta_x / delta_y : 0.0;
        break;
    }
    }

    return delta;
}

void poller::argument_check(const dynamic::argument_t& arg) const
{
    auto pollable = m_poll();
    if (!arg.x.empty()) {
        if (!pollable->has_field(arg.x))
            throw std::domain_error("Argument x with name '" + arg.x
                                    + "' not exists.");
    }

    if (!arg.y.empty()) {
        if (!pollable->has_field(arg.y))
            throw std::domain_error("Argument y with name '" + arg.y
                                    + "' not exists.");
    }
}

} // namespace openperf::dynamic
