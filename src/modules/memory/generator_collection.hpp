#ifndef _MEMORY_GENERATOR_STACK_HPP_
#define _MEMORY_GENERATOR_STACK_HPP_

#include "memory/generator.hpp"

namespace openperf::memory {

using namespace openperf::memory::internal;

class generator_collection
{
    using id_list = std::forward_list<std::string>;

private:
    std::unordered_map<std::string, generator> _collection;

public:
    generator_collection() = default;
    generator_collection(const generator_collection&) = delete;

    void start();
    void stop();
    void pause();
    void resume();

    void clear();
    void erase(const std::string& id);
    std::string create(const std::string& id,
                       const generator::config_t& config);

    class generator& generator(const std::string& id);
    const class generator& generator(const std::string& id) const;
    bool contains(const std::string& id) const;
    id_list ids() const;
};

} // namespace openperf::memory

#endif // _MEMORY_GENERATOR_STACK_HPP_
