#include "memory/generator_collection.hpp"
#include "memory/generator.hpp"
#include "core/op_core.h"
#include "config/op_config_utils.hpp"

namespace openperf::memory {

generator_configs generator_collection::list() const
{
    generator_configs list;
    for (const auto& pair : _collection) {
        list.emplace(pair.first, get(pair.first));
    }

    return list;
}

bool generator_collection::contains(const std::string& id) const
{
    // return _collection.contains(id); // C++20
    return _collection.count(id); // Legacy variant
}

generator_config generator_collection::get(const std::string& id) const
{
    auto& g = _collection.at(id);
    return generator_config::create()
        .set_buffer_size(g.read_worker_config().buffer_size)
        .set_read_threads(g.read_workers())
        .set_read_size(g.read_worker_config().block_size)
        .set_reads_per_sec(g.read_worker_config().op_per_sec)
        .set_write_threads(g.write_workers())
        .set_write_size(g.write_worker_config().block_size)
        .set_writes_per_sec(g.write_worker_config().op_per_sec)
        .set_running(g.is_running() && !g.is_paused());
}

void generator_collection::erase(const std::string& id)
{
    _collection.erase(id);
}

generator_config generator_collection::create(const std::string& id,
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

    generator gen;
    gen.set_read_workers(config.read_threads());
    gen.set_read_config(
        task_memory_read::config_t{
                     .buffer_size = config.buffer_size(),
                       .op_per_sec = config.reads_per_sec(),
                       .block_size = config.read_size(),
                       .pattern = config.pattern()});

    gen.set_write_workers(config.write_threads());
    gen.set_write_config(
        task_memory_write::config_t{
                        .buffer_size = config.buffer_size(),
                       .op_per_sec = config.writes_per_sec(),
                       .block_size = config.write_size(),
                       .pattern = config.pattern()});

    gen.set_running(config.is_running());
    auto result = _collection.emplace(new_id, std::move(gen));
    // auto result = _collection.emplace(
    //    new_id,
    //    Generator::Config{.read_threads = config.readThreads(),
    //                      .write_threads = config.writeThreads()});

    if (!result.second) {
        throw std::runtime_error(
            "Unexpected error on inserting GeneratorConfig with id '" + new_id
            + "' to collection.");
    }

    return get(new_id);
}

void generator_collection::clear() { _collection.clear(); }

void generator_collection::start()
{
    for (auto& generator : _collection) {
        generator.second.resume();
        generator.second.start();
    }
}

void generator_collection::start(const std::string& id)
{
    auto& generator = _collection.at(id);
    generator.resume();
    generator.start();
}

void generator_collection::stop()
{
    for (auto& generator : _collection) {
        generator.second.pause();
        // generator.second.stop();
    }
}

void generator_collection::stop(const std::string& id)
{
    _collection.at(id).pause();
}

} // namespace openperf::memory
