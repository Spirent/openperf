#ifndef _MEMORY_GENERATOR_STACK_HPP_
#define _MEMORY_GENERATOR_STACK_HPP_

#include <forward_list>

#include "memory/Generator.hpp"
#include "memory/GeneratorConfig.hpp"

namespace openperf::memory::generator {

typedef std::unordered_map<std::string, GeneratorConfig> GeneratorConfigList;

class GeneratorCollection
{
private:
    std::unordered_map<std::string, Generator> _collection;

public:
    GeneratorCollection() {}
    GeneratorCollection(const GeneratorCollection&) = delete;

    GeneratorConfigList list() const;
    GeneratorConfig get(const std::string& id) const;
    bool contains(const std::string& id) const;

    void erase(const std::string& id);
    GeneratorConfig create(const std::string& id,
                           const GeneratorConfig& config);
    void clear();

    void start();
    void start(const std::string& id);
    void stop();
    void stop(const std::string& id);
};

} // namespace openperf::memory::generator

#endif // _MEMORY_GENERATOR_STACK_HPP_
