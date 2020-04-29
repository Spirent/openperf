#include "generator_stack.hpp"

namespace openperf::cpu::generator {

std::vector<generator_stack::generator_ptr> generator_stack::list() const
{
    std::vector<generator_ptr> cpu_generators_list;
    for (auto cpu_generator_pair : m_generators) {
        cpu_generators_list.push_back(cpu_generator_pair.second);
    }

    return cpu_generators_list;
}

tl::expected<generator_stack::generator_ptr, std::string>
generator_stack::create(const model::generator& model)
{
    if (generator(model.id()))
        return tl::make_unexpected(
            "Generator "
            + static_cast<std::string>(model.id())
            + " already exists.");

    auto generator_ptr = std::make_shared<cpu::generator::generator>(model);
    m_generators.emplace(generator_ptr->id(), generator_ptr);
    return generator_ptr;
}

generator_stack::generator_ptr generator_stack::generator(const std::string& id) const
{
    if (m_generators.count(id)) return m_generators.at(id);
    return nullptr;
}

generator_stack::statistic_t generator_stack::statistics(const std::string& id) const
{
    if (m_statistics.count(id)) {
        auto stat = m_statistics.at(id);
        if (auto g7r = std::get_if<generator_ptr>(&stat)) {
            return statistic_t{
                .id = id,
                .generator_id = m_id_map.at(id),
                .statistics = g7r->get()->statistics()
            };
        }

        return std::get<statistic_t>(stat);
    }

    auto& g7r = m_generators.at(id);
    return statistic_t{
        .id = m_id_map.at(id),
        .generator_id = id,
        .statistics = g7r->statistics()
    };
}

void generator_stack::erase(const std::string& id)
{
    m_generators.erase(id);
}

bool generator_stack::erase_statistics(const std::string& id)
{
    auto stat = m_statistics.at(id);
    if (std::holds_alternative<statistic_t>(stat)) {
        m_statistics.erase(id);
        return true;
    }

    return false;;
}

} // namespace openperf::cpu::generator
