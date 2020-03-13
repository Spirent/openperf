#ifndef _MEMORY_GENERATOR_STACK_HPP_
#define _MEMORY_GENERATOR_STACK_HPP_

#include "swagger/v1/model/MemoryGenerator.h"
#include "memory/generator.hpp"

namespace openperf::memory::generator {

namespace model = swagger::v1::model;

typedef std::forward_list< std::reference_wrapper<const model::MemoryGenerator> > MemoryGeneratorList;

class generator_stack
{
private:
    typedef std::unordered_map<std::string, generator> generator_map;
    generator_map _collection;

public:
    const MemoryGeneratorList list() const;
    const model::MemoryGenerator& get(const std::string& id) const;
    bool contains(const std::string& id) const;

    void erase(const std::string& id);
    const model::MemoryGenerator& create(const model::MemoryGenerator& generator);
    void clear();

    void start();
    void start(const std::string& id);
    void stop();
    void stop(const std::string& id);
};

} // namespace openperf::memory::generator

#endif // _MEMORY_GENERATOR_STACK_HPP_
