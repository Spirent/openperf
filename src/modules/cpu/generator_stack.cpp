#include "generator_stack.hpp"
#include "utils/overloaded_visitor.hpp"

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

    try {
        auto generator_ptr = std::make_shared<cpu::generator::generator>(model);
        m_generators.emplace(generator_ptr->id(), generator_ptr);
        return generator_ptr;
    } catch (const std::runtime_error& e) {
        return tl::make_unexpected(e.what());
    }
}

generator_stack::generator_ptr generator_stack::generator(const std::string& id) const
{
    if (m_generators.count(id)) return m_generators.at(id);
    return nullptr;
}

tl::expected<model::generator_result, std::string> generator_stack::statistics(const std::string& id) const
{
    if (!m_statistics.count(id))
        return tl::make_unexpected("Result not found");

    auto result = m_statistics.at(id);
    auto stat = std::visit(
        utils::overloaded_visitor(
            [&](const generator_ptr& generator) {
                return generator->statistics();
            },
            [&](const model::generator_result& res) {
                return res;
            }),
    result);

    return stat;
}

std::vector<model::generator_result> generator_stack::list_statistics() const
{
    std::vector<model::generator_result> result_list;
    for (auto pair : m_statistics) {
        std::visit(
            utils::overloaded_visitor(
                [&](const generator_ptr& generator) {
                    result_list.push_back(generator->statistics());
                },
                [&](const model::generator_result& result) {
                    result_list.push_back(result);
                }),
        pair.second);
    }

    return result_list;
}

bool generator_stack::erase(const std::string& id)
{
    auto gen = generator(id);
    if (!gen)
        return false;
    if (gen->running()) {
        stop_generator(id);
    }

    return (m_generators.erase(id) > 0);
}

bool generator_stack::erase_statistics(const std::string& id)
{
    auto stat = m_statistics.at(id);
    if (std::holds_alternative<model::generator_result>(stat)) {
        m_statistics.erase(id);
        return true;
    }

    return false;;
}

tl::expected<model::generator_result, std::string> generator_stack::start_generator(const std::string& id)
{
    auto gen = generator(id);
    if (!gen)
        return tl::make_unexpected("Generator not found");

    if (gen->running()) {
        return tl::make_unexpected("Generator is already in running state");
    }

    gen->start();
    auto result = gen->statistics();
    m_statistics[result.id()] = gen;
    return result;
}

bool generator_stack::stop_generator(const std::string& id)
{
    auto gen = generator(id);
    if (!gen)
        return false;

    if (!gen->running()) {
        return true;
    }

    gen->stop();
    auto result = gen->statistics();
    m_statistics[result.id()] = result;
    gen->clear_statistics();
    return true;
}

} // namespace openperf::cpu::generator
