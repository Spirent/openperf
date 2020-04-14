#include "generator_stack.hpp"

namespace openperf::cpu::generator {

std::vector<cpu_generator_ptr> generator_stack::cpu_generators_list() const
{
    std::vector<cpu_generator_ptr> cpu_generators_list;
    for (auto cpu_generator_pair : m_generators) {
        cpu_generators_list.push_back(cpu_generator_pair.second);
    }

    return cpu_generators_list;
}

tl::expected<cpu_generator_ptr, std::string>
generator_stack::create_cpu_generator(const model::cpu_generator& cpu_generator_model)
{
    if (get_cpu_generator(cpu_generator_model.get_id()))
        return tl::make_unexpected(
            "Generator "
            + static_cast<std::string>(cpu_generator_model.get_id())
            + " already exists.");

    auto cpu_generator_ptr = std::make_shared<cpu_generator>(cpu_generator_model);
    m_generators.emplace(cpu_generator_ptr->get_id(), cpu_generator_ptr);
    return cpu_generator_ptr;

}

cpu_generator_ptr generator_stack::get_cpu_generator(std::string id) const
{
    if (m_generators.count(id)) return m_generators.at(id);
    return nullptr;
}

void generator_stack::delete_cpu_generator(std::string id)
{
    m_generators.erase(id);
}

} // namespace openperf::cpu::generator
