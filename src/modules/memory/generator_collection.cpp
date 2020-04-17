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
    stop(id);
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
    _id_map.clear();
}

void generator_collection::start()
{
    for (auto& entry : _generators) {
        auto& g7r = entry.second;
        if (g7r.is_stopped()) {
            auto stat_id = core::to_string(core::uuid::random());
            _stats.emplace(stat_id, generator_ref(g7r));
            _id_map.insert_or_assign(entry.first, stat_id);
            _id_map.insert_or_assign(stat_id, entry.first);
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
        _id_map.insert_or_assign(id, stat_id);
        _id_map.insert_or_assign(stat_id, id);
        g7r.start();
    }
}

void generator_collection::stop()
{
    for (auto& entry : _generators) {
        auto& g7r = entry.second;
        if (!g7r.is_stopped()) {
            auto stat_id = _id_map.at(entry.first);
            g7r.stop();
            _stats.insert_or_assign(stat_id,
                                    stat_t{.id = stat_id,
                                           .generator_id = entry.first,
                                           .stat = g7r.stat()});
            _id_map.erase(entry.first);
            g7r.reset();
        }
    }
}

void generator_collection::stop(const std::string& id)
{
    auto& g7r = _generators.at(id);
    if (!g7r.is_stopped()) {
        auto stat_id = _id_map.at(id);
        g7r.stop();
        _stats.insert_or_assign(
            stat_id,
            stat_t{.id = stat_id, .generator_id = id, .stat = g7r.stat()});
        _id_map.erase(id);
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

generator_collection::stat_t
generator_collection::stat(const std::string& id) const
{
    if (_stats.count(id)) {
        auto stat = _stats.at(id);
        if (auto g7r = std::get_if<generator_ref>(&stat)) {
            return stat_t{.id = id,
                          .generator_id = _id_map.at(id),
                          .stat = g7r->get().stat()};
        }

        return std::get<stat_t>(stat);
    }

    auto& g7r = _generators.at(id);
    return stat_t{.id = _id_map.at(id), .generator_id = id, .stat = g7r.stat()};
}

generator_collection::stat_list generator_collection::stats() const
{
    auto stat_converter = [this](auto& pair) {
        if (auto g7r = std::get_if<generator_ref>(&pair.second)) {
            return stat_t{.id = pair.first,
                          .generator_id = _id_map.at(pair.first),
                          .stat = g7r->get().stat()};
        }

        return std::get<stat_t>(pair.second);
    };

    stat_list list;
    std::transform(_stats.begin(),
                   _stats.end(),
                   std::front_inserter(list),
                   stat_converter);
    return list;
}

bool generator_collection::erase_stat(const std::string& id)
{
    //_stats.erase(id);
    // if (_id_map.contains(id)) {
    //    auto g7r = std::get<generator_ref>(_stats.at(id));
    //    auto stat_id = core::to_string(core::uuid::random());

    //    g7r.get().reset();
    //    _stats.emplace(stat_id, generator_ref(g7r));
    //    _id_map.insert(
    //        _id_map.at(id), stat_id);
    //}
    auto stat = _stats.at(id);
    if (std::holds_alternative<stat_t>(stat)) {
        _stats.erase(id);
        return true;
    }

    return false;
}

} // namespace openperf::memory
