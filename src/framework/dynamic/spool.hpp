#ifndef _OP_DYNAMIC_POOL_HPP_
#define _OP_DYNAMIC_POOL_HPP_

#include "api.hpp"
#include "framework/core/op_uuid.hpp"
#include "framework/config/op_config_utils.hpp"
#include "digestible/digestible.h"

#include <map>
#include <optional>
#include <string_view>

namespace openperf::dynamic {

template <typename T> class spool
{
public:
    using extractor = std::optional<double> (*)(const T&, std::string_view);
    using tdigest = ::digestible::tdigest<double, uint32_t>;

private:
    // Internal Types
    struct threshold_arg
    {
        argument_t argument;
        threshold threshold;
    };

    struct tdigest_arg
    {
        argument_t argument;
        uint32_t compression;
        tdigest tdigest;
    };

    // Attributes
    std::map<std::string, threshold_arg> m_thresholds;
    std::map<std::string, tdigest_arg> m_tdigests;
    extractor m_extractor;
    T m_last_stat;

public:
    explicit spool(extractor&& f);

    configuration config() const;
    results result() const;

    void reset();
    void configure(const configuration&, const T& = {});
    void add(const T& stat);

private:
    uint32_t weight(const argument_t&, const T&) const;
    double delta(const argument_t&, const T&) const;
    void argument_check(const argument_t&) const;
};

//
// Implementation
//

// Constructors & Destructor
template <typename T>
spool<T>::spool(spool::extractor&& f)
    : m_extractor(f)
{}

// Methods : public
template <typename T>
void spool<T>::configure(const configuration& config, const T& stat)
{
    m_last_stat = stat;

    auto get_id = [](const std::string& s) {
        auto id = s;
        if (id.empty()) id = core::to_string(core::uuid::random());
        if (auto check = config::op_config_validate_id_string(id); !check)
            throw std::domain_error(check.error().c_str());

        return id;
    };

    m_thresholds.clear();
    for (const auto& cfg : config.thresholds) {
        argument_check(cfg.argument);

        auto id = get_id(cfg.id);
        if (m_thresholds.count(id)) {
            throw std::invalid_argument("Item with ID '" + id
                                        + "' already exists.");
        }

        m_thresholds.emplace(id,
                             threshold_arg{
                                 .argument = cfg.argument,
                                 .threshold = {cfg.value, cfg.condition},
                             });
    }

    m_tdigests.clear();
    for (const auto& cfg : config.tdigests) {
        argument_check(cfg.argument);

        auto id = get_id(cfg.id);
        if (m_tdigests.count(id)) {
            throw std::invalid_argument("Item with ID '" + id
                                        + "' already exists.");
        }

        m_tdigests.emplace(id,
                           tdigest_arg{
                               .argument = cfg.argument,
                               .compression = cfg.compression,
                               .tdigest = tdigest(cfg.compression),
                           });
    }
}

template <typename T> configuration spool<T>::config() const
{
    configuration conf;
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

template <typename T> void spool<T>::reset()
{
    std::for_each(m_thresholds.begin(), m_thresholds.end(), [](auto& x) {
        x.second.threshold.reset();
    });

    std::for_each(m_tdigests.begin(), m_tdigests.end(), [](auto& x) {
        x.second.tdigest.reset();
    });
}

template <typename T> void spool<T>::add(const T& stat)
{
    for (auto& th : m_thresholds) {
        auto& data = th.second;
        data.threshold.append(delta(data.argument, stat));
    }

    for (auto& td : m_tdigests) {
        auto& data = td.second;
        data.tdigest.insert(delta(data.argument, stat),
                            weight(data.argument, stat));
    }

    m_last_stat = stat;
}

template <typename T> results spool<T>::result() const
{
    results result;
    std::transform(m_thresholds.begin(),
                   m_thresholds.end(),
                   std::back_inserter(result.thresholds),
                   [](const auto& pair) {
                       auto& data = pair.second;
                       return results::threshold{
                           .argument = data.argument,
                           .id = pair.first,
                           .threshold = data.threshold,
                       };
                   });

    std::transform(m_tdigests.begin(),
                   m_tdigests.end(),
                   std::back_inserter(result.tdigests),
                   [](const auto& pair) {
                       auto& data = pair.second;
                       return results::tdigest{
                           .argument = data.argument,
                           .id = pair.first,
                           .compression = data.compression,
                           .centroids = data.tdigest.get(),
                       };
                   });

    return result;
}

// Methods : private
template <typename T>
uint32_t spool<T>::weight(const argument_t& arg, const T& stat) const
{
    int weight = 1;
    switch (arg.function) {
    case argument_t::DXDY: {
        auto y = m_extractor(stat, arg.y).value();
        auto last_y = m_extractor(m_last_stat, arg.y).value();
        weight = static_cast<uint32_t>(y - last_y);
        break;
    }
    default:
        break;
    }

    return weight;
}

template <typename T>
double spool<T>::delta(const argument_t& arg, const T& stat) const
{
    auto x = m_extractor(stat, arg.x).value();
    auto last_x = m_extractor(m_last_stat, arg.x).value();
    auto delta_x = x - last_x;

    double delta = 0;
    switch (arg.function) {
    case argument_t::DX: {
        delta = delta_x;
        break;
    }
    case argument_t::DXDT: {
        auto t = m_extractor(stat, "timestamp").value();
        auto last_t = m_extractor(m_last_stat, "timestamp").value();
        auto delta_t = t - last_t;
        delta = (delta_t != 0.0) ? delta_x / delta_t : 0.0;
        break;
    }
    case argument_t::DXDY: {
        auto y = m_extractor(stat, arg.y).value();
        auto last_y = m_extractor(m_last_stat, arg.y).value();
        auto delta_y = y - last_y;
        delta = (delta_y != 0.0) ? delta_x / delta_y : 0.0;
        break;
    }
    }

    return delta;
}

template <typename T> void spool<T>::argument_check(const argument_t& arg) const
{
    if (!m_extractor(m_last_stat, arg.x)) {
        throw std::domain_error("Argument x with name '" + arg.x
                                + "' does not exist.");
    }

    switch (arg.function) {
    case argument_t::DXDY:
        if (!m_extractor(m_last_stat, arg.y)) {
            throw std::domain_error("Argument y with name '" + arg.y
                                    + "' does not exist.");
        }
        break;
    case argument_t::DXDT:
        if (!m_extractor(m_last_stat, "timestamp")) {
            throw std::domain_error(
                "Field with name 'timestamp' does not exist.");
        }
        break;
    default:
        break;
    }
}

} // namespace openperf::dynamic

#endif // _OP_DYNAMIC_POOL_HPP_
