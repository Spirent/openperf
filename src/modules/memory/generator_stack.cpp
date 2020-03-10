#include "memory/generator_stack.hpp"

namespace openperf::memory::generator {

std::vector<MemoryGeneratorPointer> generator_stack::list() const
{
    std::vector<MemoryGeneratorPointer> list;
    for (auto& pair : generators) list.push_back(pair.second);

    return list;
}

bool generator_stack::contains(const std::string& id) const
{
    // return generators.contains(id); // C++20
    return generators.count(id); // Legacy variant
}

MemoryGeneratorPointer generator_stack::get(const std::string& id) const
{
    try {
        return generators.at(id);
    } catch (std::out_of_range) {
        return nullptr;
    }
}

void generator_stack::erase(const std::string& id) { generators.erase(id); }

tl::expected<MemoryGeneratorPointer, std::string>
generator_stack::create(const MemoryGenerator& generator)
{
    if (contains(generator.getId()))
        return tl::make_unexpected("Memory generator with id '"
                                   + generator.getId() + "' already exists.");

    auto ptr = MemoryGeneratorPointer(new MemoryGenerator(generator));
    generators.emplace(generator.getId(), ptr);

    return ptr;
}

} // namespace openperf::memory::generator