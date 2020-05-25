#include "memory/generator_collection.hpp"
#include "memory/generator.hpp"
#include "core/op_core.h"
#include "config/op_config_utils.hpp"

namespace openperf::memory {

generator_collection::id_list generator_collection::ids() const
{
    id_list list;
    std::transform(m_generators.begin(),
                   m_generators.end(),
                   std::front_inserter(list),
                   [](auto& entry) { return entry.first; });
    return list;
}

bool generator_collection::contains(const std::string& id) const
{
    // return m_generators.contains(id); // C++20
    return m_generators.count(id); // Legacy variant
}

bool generator_collection::contains_stat(const std::string& id) const
{
    // return m_generators.contains(id); // C++20
    return m_stats.count(id); // Legacy variant
}

const generator& generator_collection::generator(const std::string& id) const
{
    return m_generators.at(id);
}

// generator& generator_collection::generator(const std::string& id)
//{
//    return m_generators.at(id);
//}

void generator_collection::erase(const std::string& id)
{
    stop(id);
    m_generators.erase(id);
}

std::string generator_collection::create(const std::string& id,
                                         const generator::config_t& config)
{
    auto gid = [this](const std::string& id) {
        if (id.empty()) return core::to_string(core::uuid::random());

        if (auto id_check = config::op_config_validate_id_string(id); !id_check)
            throw std::domain_error(id_check.error().c_str());

        if (contains(id))
            throw std::invalid_argument("Memory generator with ID '" + id
                                        + "' already exists.");

        return id;
    };

    auto result = m_generators.emplace(gid(id), config);
    if (!result.second) {
        throw std::runtime_error(
            "Unexpected error on inserting GeneratorConfig with id '" + id
            + "' to collection.");
    }

    return result.first->first;
}

void generator_collection::clear()
{
    m_generators.clear();
    m_stats.clear();
    m_id_map.clear();
}

void generator_collection::start()
{
    for (auto& entry : m_generators) {
        auto& g7r = entry.second;
        if (g7r.is_stopped()) {
            auto stat_id = core::to_string(core::uuid::random());
            m_stats.emplace(stat_id, generator_ref(g7r));
            m_id_map.insert_or_assign(entry.first, stat_id);
            m_id_map.insert_or_assign(stat_id, entry.first);
            entry.second.start();
        }
    }
}

void generator_collection::start(const std::string& id)
{
    auto& g7r = m_generators.at(id);
    if (g7r.is_stopped()) {
        auto stat_id = core::to_string(core::uuid::random());
        m_stats.emplace(stat_id, generator_ref(g7r));
        m_id_map.insert_or_assign(id, stat_id);
        m_id_map.insert_or_assign(stat_id, id);
        g7r.start();
    }
}

void generator_collection::stop()
{
    for (auto& entry : m_generators) {
        auto& g7r = entry.second;
        if (!g7r.is_stopped()) {
            auto stat_id = m_id_map.at(entry.first);
            g7r.stop();
            m_stats.insert_or_assign(stat_id,
                                     stat_t{.id = stat_id,
                                            .generator_id = entry.first,
                                            .stat = g7r.stat()});
            m_id_map.erase(entry.first);
            g7r.reset();
        }
    }
}

void generator_collection::stop(const std::string& id)
{
    auto& g7r = m_generators.at(id);
    if (!g7r.is_stopped()) {
        auto stat_id = m_id_map.at(id);
        g7r.stop();
        m_stats.insert_or_assign(
            stat_id,
            stat_t{.id = stat_id, .generator_id = id, .stat = g7r.stat()});
        m_id_map.erase(id);
        g7r.reset();
    }
}

void generator_collection::pause()
{
    for (auto& entry : m_generators) { entry.second.pause(); }
}

void generator_collection::pause(const std::string& id)
{
    m_generators.at(id).pause();
}

void generator_collection::resume()
{
    for (auto& entry : m_generators) { entry.second.resume(); }
}

void generator_collection::resume(const std::string& id)
{
    m_generators.at(id).resume();
}

generator_collection::stat_t
generator_collection::stat(const std::string& id) const
{
    if (m_stats.count(id)) {
        auto stat = m_stats.at(id);
        if (auto g7r = std::get_if<generator_ref>(&stat)) {
            return stat_t{.id = id,
                          .generator_id = m_id_map.at(id),
                          .stat = g7r->get().stat()};
        }

        return std::get<stat_t>(stat);
    }

    auto& g7r = m_generators.at(id);
    return stat_t{
        .id = m_id_map.at(id), .generator_id = id, .stat = g7r.stat()};
}

generator_collection::stat_list generator_collection::stats() const
{
    auto stat_converter = [this](auto& pair) {
        if (auto g7r = std::get_if<generator_ref>(&pair.second)) {
            return stat_t{.id = pair.first,
                          .generator_id = m_id_map.at(pair.first),
                          .stat = g7r->get().stat()};
        }

        return std::get<stat_t>(pair.second);
    };

    stat_list list;
    std::transform(m_stats.begin(),
                   m_stats.end(),
                   std::front_inserter(list),
                   stat_converter);
    return list;
}

bool generator_collection::erase_stat(const std::string& id)
{
    // m_stats.erase(id);
    // if (m_id_map.contains(id)) {
    //    auto g7r = std::get<generator_ref>(m_stats.at(id));
    //    auto stat_id = core::to_string(core::uuid::random());

    //    g7r.get().reset();
    //    m_stats.emplace(stat_id, generator_ref(g7r));
    //    m_id_map.insert(
    //        m_id_map.at(id), stat_id);
    //}
    auto stat = m_stats.at(id);
    if (std::holds_alternative<stat_t>(stat)) {
        m_stats.erase(id);
        return true;
    }

    return false;
}

} // namespace openperf::memory
