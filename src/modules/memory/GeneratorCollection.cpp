#include "GeneratorCollection.hpp"
#include "Generator.hpp"
#include "core/op_core.h"
#include "config/op_config_utils.hpp"

namespace openperf::memory::generator {

GeneratorConfigList GeneratorCollection::list() const
{
    GeneratorConfigList list;
    for (const auto& pair : _collection) {
        list.push_front(collectConfig(pair.first));
    }

    return list;
}

bool GeneratorCollection::contains(const std::string& id) const
{
    // return _collection.contains(id); // C++20
    return _collection.count(id); // Legacy variant
}

GeneratorConfig GeneratorCollection::get(const std::string& id) const
{
    // auto config = _collection.at(id).config();
    return collectConfig(id);
}

void GeneratorCollection::erase(const std::string& id)
{
    _collection.erase(id);
}

GeneratorConfig GeneratorCollection::create(const std::string& id,
                                            const GeneratorConfig& config)
{
    auto id_check = config::op_config_validate_id_string(id);
    if (!id_check) { throw std::invalid_argument(id_check.error().c_str()); }

    auto new_id = id;
    if (new_id == core::empty_id_string) {
        new_id = core::to_string(core::uuid::random());
    } else if (contains(new_id)) {
        throw std::invalid_argument("Memory generator with id '" + new_id
                                    + "' already exists.");
    }

    auto result = _collection.emplace(
        new_id,
        Generator::Config{.read_threads = config.readThreads(),
                          .write_threads = config.writeThreads()});

    if (!result.second) {
        throw std::runtime_error(
            "Unexpected error on inserting GeneratorConfig with id '" + new_id
            + "' to collection.");
    }

    return collectConfig(new_id);
}

void GeneratorCollection::clear() { _collection.clear(); }

void GeneratorCollection::start()
{
    for (auto& generator : _collection) {
        generator.second.resume();
        // generator.second.start();
    }
}

void GeneratorCollection::start(const std::string& id)
{
    _collection.at(id).resume();
}

void GeneratorCollection::stop()
{
    for (auto& generator : _collection) {
        generator.second.pause();
        // generator.second.stop();
    }
}

void GeneratorCollection::stop(const std::string& id)
{
    _collection.at(id).pause();
}

GeneratorConfig GeneratorCollection::collectConfig(const std::string& id) const
{
    GeneratorConfig config;
    auto& g = _collection.at(id);

    config.setReadThreads(g.readThreads());
    config.setWriteThreads(g.writeThreads());

    return config;
}

} // namespace openperf::memory::generator
