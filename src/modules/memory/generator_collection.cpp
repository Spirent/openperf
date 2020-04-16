#include "memory/generator_collection.hpp"
#include "memory/generator.hpp"
#include "core/op_core.h"
#include "config/op_config_utils.hpp"

namespace openperf::memory {

generator_collection::id_list generator_collection::ids() const
{
    id_list list;
    std::transform(_generators.begin(),
                   _generators.end(),
                   std::front_inserter(list),
                   [](auto& entry) { return entry.first; });
    return list;
}

generator_collection::id_list generator_collection::stats() const
{
    id_list list;
    std::transform(_stats.begin(),
                   _stats.end(),
                   std::front_inserter(list),
                   [](auto& entry) { return entry.first; });
    return list;
}

bool generator_collection::contains(const std::string& id) const
{
    // return _generators.contains(id); // C++20
    return _generators.count(id); // Legacy variant
}

bool generator_collection::contains_stat(const std::string& id) const
{
    // return _generators.contains(id); // C++20
    return _stats.count(id); // Legacy variant
}

const generator& generator_collection::generator(const std::string& id) const
{
    return _generators.at(id);
}

// generator& generator_collection::generator(const std::string& id)
//{
//    return _generators.at(id);
//}

void generator_collection::erase(const std::string& id)
{
    _generators.erase(id);
}

std::string generator_collection::create(const std::string& id,
                                         const generator::config_t& config)
{
    auto gid = [this](const std::string& id) {
        if (id.empty()) return core::to_string(core::uuid::random());

        if (auto id_check = config::op_config_validate_id_string(id); !id_check)
            throw std::domain_error(id_check.error().c_str());

        if (contains(id))
            throw std::invalid_argument("Memory generator with id '" + id
                                        + "' already exists.");

        return id;
    };

    auto result = _generators.emplace(gid(id), config);
    if (!result.second) {
        throw std::runtime_error(
            "Unexpected error on inserting GeneratorConfig with id '" + id
            + "' to collection.");
    }

    return result.first->first;
}

void generator_collection::clear()
{
    _generators.clear();
    _stats.clear();
    _active_stats.clear();
}

void generator_collection::start()
{
    for (auto& entry : _generators) {
        auto& g7r = entry.second;
        if (g7r.is_stopped()) {
            auto stat_id = core::to_string(core::uuid::random());
            _stats.emplace(stat_id, generator_ref(g7r));
            _active_stats.insert(entry.first, stat_id);
            entry.second.start();
        }
    }
}

void generator_collection::start(const std::string& id)
{
    auto& g7r = _generators.at(id);
    if (g7r.is_stopped()) {
        auto stat_id = core::to_string(core::uuid::random());
        _stats.emplace(stat_id, generator_ref(g7r));
        _active_stats.insert(id, stat_id);
        g7r.start();
    }
}

void generator_collection::stop()
{
    for (auto& entry : _generators) {
        auto& g7r = entry.second;
        if (!g7r.is_stopped()) {
            g7r.stop();
            _stats.insert_or_assign(_active_stats.at(entry.first), g7r.stat());
            _active_stats.erase(entry.first);
            g7r.reset();
        }
    }
}

void generator_collection::stop(const std::string& id)
{
    auto& g7r = _generators.at(id);
    if (!g7r.is_stopped()) {
        g7r.stop();
        _stats.insert_or_assign(_active_stats.at(id), g7r.stat());
        _active_stats.erase(id);
        g7r.reset();
    }
}

void generator_collection::pause()
{
    for (auto& entry : _generators) { entry.second.pause(); }
}

void generator_collection::pause(const std::string& id)
{
    _generators.at(id).pause();
}

void generator_collection::resume()
{
    for (auto& entry : _generators) { entry.second.resume(); }
}

void generator_collection::resume(const std::string& id)
{
    _generators.at(id).resume();
}

generator::stat_t generator_collection::stat(const std::string& id) const
{
    auto stat = _stats.at(id);
    if (auto g7r = std::get_if<generator_ref>(&stat)) {
        return g7r->get().stat();
    }

    return std::get<generator::stat_t>(stat);
}

bool generator_collection::erase_stat(const std::string& id)
{
    //_stats.erase(id);
    // if (_active_stats.contains(id)) {
    //    auto g7r = std::get<generator_ref>(_stats.at(id));
    //    auto stat_id = core::to_string(core::uuid::random());

    //    g7r.get().reset();
    //    _stats.emplace(stat_id, generator_ref(g7r));
    //    _active_stats.insert(
    //        _active_stats.at(id), stat_id);
    //}
    auto stat = _stats.at(id);
    if (std::holds_alternative<generator::stat_t>(stat)) {
        _stats.erase(id);
        return true;
    }

    return false;
}

} // namespace openperf::memory
