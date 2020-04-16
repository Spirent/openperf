#ifndef _MEMORY_GENERATOR_STACK_HPP_
#define _MEMORY_GENERATOR_STACK_HPP_

#include <variant>

#include "memory/generator.hpp"

namespace openperf::memory {

using namespace openperf::memory::internal;

class generator_collection
{
    using id_list = std::forward_list<std::string>;
    using generator_ref = std::reference_wrapper<generator>;
    using stat_t = std::variant<generator_ref, generator::stat_t>;

    class id_junction
    {
        std::unordered_map<std::string, std::string> _map;

    public:
        inline void clear() { _map.clear(); }

        void insert(const std::string& id1, const std::string& id2)
        {
            erase(id1);
            erase(id2);
            _map.insert_or_assign(id1, id2);
            _map.insert_or_assign(id2, id1);
        }

        void erase(const std::string& id)
        {
            if (_map.count(id)) {
                _map.erase(_map.at(id));
                _map.erase(id);
            }
        }

        inline bool contains(const std::string& id) const
        {
            return _map.count(id);
        }
        std::string at(const std::string& id) const { return _map.at(id); }
    };

private:
    std::unordered_map<std::string, generator> _generators;
    std::unordered_map<std::string, stat_t> _stats;
    id_junction _active_stats;

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

    generator::stat_t stat(const std::string& id) const;
    const class generator& generator(const std::string& id) const;
    // class generator& generator(const std::string& id);
    bool contains(const std::string& id) const;
    bool contains_stat(const std::string& id) const;
    id_list ids() const;
    id_list stats() const;

private:
};

} // namespace openperf::memory

#endif // _MEMORY_GENERATOR_STACK_HPP_
