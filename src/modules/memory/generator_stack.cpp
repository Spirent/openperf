#include "generator_stack.hpp"
#include "generator.hpp"
#include "core/op_core.h"
#include "config/op_config_utils.hpp"

namespace openperf::memory::generator {

const MemoryGeneratorList generator_stack::list() const
{
    MemoryGeneratorList list;
    for (auto& pair : _collection) { 
        list.push_front(pair.second.model()); 
    }

    return list;
}

bool generator_stack::contains(const std::string& id) const
{
    // return _collection.contains(id); // C++20
    return _collection.count(id); // Legacy variant
}

const model::MemoryGenerator& generator_stack::get(const std::string& id) const
{
    return _collection.at(id).model();
}

void generator_stack::erase(const std::string& id) { 
    _collection.erase(id); 
}

const model::MemoryGenerator& generator_stack::create(const model::MemoryGenerator& g_model)
{
    auto id_check = config::op_config_validate_id_string(g_model.getId());
    if (!id_check) { 
        throw std::invalid_argument(
            id_check.error().c_str()); 
    }

    auto internal_model = g_model;
    if (internal_model.getId() == core::empty_id_string) {
        internal_model.setId(core::to_string(core::uuid::random()));
    } else if (contains(internal_model.getId())) {
        throw std::invalid_argument(
            "Memory generator with id '"
            + internal_model.getId() + "' already exists.");
    }

    auto result = _collection.emplace(internal_model.getId(), internal_model);
    if (!result.second) {
        throw std::runtime_error(
            "Unexpected error on inserting MemoryGenerator with id '"
            + internal_model.getId() + "' to collection.");
    }

    return result.first->second.model();
}

void generator_stack::clear()
{
    _collection.clear();
}

void generator_stack::start()
{
    for (auto& generator : _collection) {
        generator.second.start();
    }
}

void generator_stack::start(const std::string& id)
{
    _collection.at(id).start();
}

void generator_stack::stop()
{
    for (auto& generator : _collection) {
        generator.second.stop();
    }
}

void generator_stack::stop(const std::string& id)
{
    _collection.at(id).stop();
}

} // namespace openperf::memory::generator
