#include "memory/generator_collection.hpp"
#include "memory/generator.hpp"
#include "core/op_core.h"
#include "config/op_config_utils.hpp"

namespace openperf::memory {

generator_collection::id_list generator_collection::ids() const
{
    id_list list;
    for (const auto& pair : _collection) { list.push_front(pair.first); }

    return list;
}

bool generator_collection::contains(const std::string& id) const
{
    // return _collection.contains(id); // C++20
    return _collection.count(id); // Legacy variant
}

const generator& generator_collection::generator(const std::string& id) const
{
    return _collection.at(id);
}

generator& generator_collection::generator(const std::string& id)
{
    return _collection.at(id);
}

void generator_collection::erase(const std::string& id)
{
    _collection.erase(id);
}

std::string generator_collection::create(const std::string& id,
                                         const generator::config_t& config)
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

    auto result = _collection.emplace(new_id, config);

    if (!result.second) {
        throw std::runtime_error(
            "Unexpected error on inserting GeneratorConfig with id '" + new_id
            + "' to collection.");
    }

    return new_id;
}

void generator_collection::clear() { _collection.clear(); }

void generator_collection::start()
{
    for (auto& generator : _collection) { generator.second.start(); }
}

void generator_collection::stop()
{
    for (auto& generator : _collection) { generator.second.stop(); }
}

void generator_collection::pause()
{
    for (auto& generator : _collection) { generator.second.pause(); }
}

void generator_collection::resume()
{
    for (auto& generator : _collection) { generator.second.resume(); }
}

} // namespace openperf::memory
