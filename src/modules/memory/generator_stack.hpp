#ifndef _MEMORY_GENERATOR_STACK_HPP_
#define _MEMORY_GENERATOR_STACK_HPP_

#include "swagger/v1/model/MemoryGenerator.h"
#include "tl/expected.hpp"

namespace openperf::memory::generator {

using namespace swagger::v1::model;

typedef std::shared_ptr<MemoryGenerator> MemoryGeneratorPointer;

class generator_stack
{
private:
    typedef std::unordered_map<std::string, MemoryGeneratorPointer>
        MemoryGeneratorMap;
    MemoryGeneratorMap generators;

public:
    std::vector<MemoryGeneratorPointer> list() const;
    MemoryGeneratorPointer get(const std::string& id) const;
    bool contains(const std::string& id) const;

    void erase(const std::string& id);
    tl::expected<MemoryGeneratorPointer, std::string>
    create(const MemoryGenerator& generator);
};

} // namespace openperf::memory::generator

#endif // _MEMORY_GENERATOR_STACK_HPP_