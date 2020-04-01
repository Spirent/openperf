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

generator_config generator_collection::config(const std::string& id) const
{
    auto& g = _collection.at(id);
    return generator_config::create()
        .buffer_size(g.read_worker_config().buffer_size)
        .pattern(g.read_worker_config().pattern)
        .read_threads(g.read_workers())
        .read_size(g.read_worker_config().block_size)
        .reads_per_sec(g.read_worker_config().op_per_sec)
        .write_threads(g.write_workers())
        .write_size(g.write_worker_config().block_size)
        .writes_per_sec(g.write_worker_config().op_per_sec)
        .running(g.is_running() && !g.is_paused());
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
                                         const generator_config& config)
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

    class generator gen;
    gen.read_workers(config.read_threads());
    gen.read_config(
        task_memory_read::config_t{.buffer_size = config.buffer_size(),
                                   .op_per_sec = config.reads_per_sec(),
                                   .block_size = config.read_size(),
                                   .pattern = config.pattern()});

    gen.write_workers(config.write_threads());
    gen.write_config(
        task_memory_write::config_t{.buffer_size = config.buffer_size(),
                                    .op_per_sec = config.writes_per_sec(),
                                    .block_size = config.write_size(),
                                    .pattern = config.pattern()});
    gen.running(config.is_running());
    auto result = _collection.emplace(new_id, std::move(gen));

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
    for (auto& generator : _collection) {
        generator.second.resume();
        generator.second.start();
    }
}

void generator_collection::stop()
{
    for (auto& generator : _collection) {
        generator.second.pause();
        // generator.second.stop();
    }
}

} // namespace openperf::memory
