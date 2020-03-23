#ifndef _MEMORY_GENERATOR_STACK_HPP_
#define _MEMORY_GENERATOR_STACK_HPP_

#include <forward_list>

#include "memory/generator.hpp"
#include "memory/generator_config.hpp"

namespace openperf::memory {

using namespace openperf::memory::internal;

typedef std::unordered_map<std::string, generator_config> generator_configs;

class generator_collection
{
private:
    std::unordered_map<std::string, generator> _collection;

public:
    generator_collection() {}
    generator_collection(const generator_collection&) = delete;

    generator_configs list() const;
    generator_config get(const std::string& id) const;
    bool contains(const std::string& id) const;

    void erase(const std::string& id);
    generator_config create(const std::string& id,
                           const generator_config& config);
    void clear();

    void start();
    void start(const std::string& id);
    void stop();
    void stop(const std::string& id);
};

} // namespace openperf::memory::generator

#endif // _MEMORY_GENERATOR_STACK_HPP_
