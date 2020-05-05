#ifndef _MEMORY_GENERATOR_STACK_HPP_
#define _MEMORY_GENERATOR_STACK_HPP_

#include <variant>

#include "memory/generator.hpp"

namespace openperf::memory {

using namespace openperf::memory::internal;

class generator_collection
{
public:
    struct stat_t
    {
        std::string id;
        std::string generator_id;
        generator::stat_t stat;
    };

    using stat_list = std::forward_list<stat_t>;

private:
    using id_list = std::forward_list<std::string>;
    using generator_ref = std::reference_wrapper<generator>;
    using stat_variant = std::variant<generator_ref, stat_t>;

private:
    std::unordered_map<std::string, generator> m_generators;
    std::unordered_map<std::string, stat_variant> m_stats;
    std::unordered_map<std::string, std::string> m_id_map;

public:
    generator_collection() = default;
    generator_collection(const generator_collection&) = delete;

    void start();
    void start(const std::string& id);
    void stop();
    void stop(const std::string& id);
    void pause();
    void pause(const std::string& id);
    void resume();
    void resume(const std::string& id);

    void clear();
    void erase(const std::string& id);
    bool erase_stat(const std::string& id);
    std::string create(const std::string& id,
                       const generator::config_t& config);

    stat_list stats() const;
    stat_t stat(const std::string& id) const;
    bool contains_stat(const std::string& id) const;

    const class generator& generator(const std::string& id) const;
    // class generator& generator(const std::string& id);
    bool contains(const std::string& id) const;
    id_list ids() const;

private:
};

} // namespace openperf::memory

#endif // _MEMORY_GENERATOR_STACK_HPP_
