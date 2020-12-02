#include "generator_stack.hpp"

#include "framework/utils/overloaded_visitor.hpp"

namespace openperf::network::internal {

std::vector<generator_stack::generator_ptr> generator_stack::list() const
{
    std::vector<generator_ptr> network_generators_list;
    std::transform(m_generators.begin(),
                   m_generators.end(),
                   std::back_inserter(network_generators_list),
                   [](const auto& i) { return i.second; });

    return network_generators_list;
}

tl::expected<generator_stack::generator_ptr, std::string>
generator_stack::create(const model::generator& model)
{
    if (m_generators.count(model.id()))
        return tl::make_unexpected("Generator with ID " + model.id()
                                   + " already exists.");

    try {
        auto generator_ptr = std::make_shared<class generator>(model);
        m_generators.emplace(generator_ptr->id(), generator_ptr);
        return generator_ptr;
    } catch (const std::runtime_error& e) {
        return tl::make_unexpected(e.what());
    }
}

generator_stack::generator_ptr
generator_stack::generator(const std::string& id) const
{
    if (auto it = m_generators.find(id); it != m_generators.end())
        return it->second;

    return nullptr;
}

tl::expected<model::generator_result, std::string>
generator_stack::statistics(const std::string& id) const
{
    try {
        auto result = m_statistics.at(id);
        return std::visit(
            utils::overloaded_visitor(
                [&](const generator_ptr& generator) {
                    return generator->statistics();
                },
                [&](const model::generator_result& res) { return res; }),
            result);
    } catch (const std::out_of_range&) {
        return tl::make_unexpected("Result not found");
    }
}

std::vector<model::generator_result> generator_stack::list_statistics() const
{
    std::vector<model::generator_result> result_list;
    for (const auto& pair : m_statistics) {
        std::visit(utils::overloaded_visitor(
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
    try {
        auto gen = m_generators.at(id);
        if (gen->running()) stop_generator(id);

        return m_generators.erase(id);
    } catch (const std::out_of_range&) {
        return false;
    }
}

bool generator_stack::erase_statistics(const std::string& id)
{
    try {
        auto stat = m_statistics.at(id);
        if (std::holds_alternative<model::generator_result>(stat))
            return m_statistics.erase(id);

        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

tl::expected<model::generator_result, std::string>
generator_stack::start_generator(const std::string& id)
{
    try {
        auto gen = m_generators.at(id);
        if (gen->running())
            return tl::make_unexpected("Generator is already in running state");

        gen->start();
        auto result = gen->statistics();
        m_statistics[result.id()] = gen;
        return result;
    } catch (const std::out_of_range&) {
        return tl::make_unexpected("Generator not found");
    }
}

tl::expected<model::generator_result, std::string>
generator_stack::start_generator(const std::string& id,
                                 const dynamic::configuration& cfg)
{
    try {
        auto gen = m_generators.at(id);
        if (gen->running())
            return tl::make_unexpected("Generator is already in running state");

        gen->start(cfg);
        auto result = gen->statistics();
        m_statistics[result.id()] = gen;
        return result;
    } catch (const std::out_of_range&) {
        return tl::make_unexpected("Generator not found");
    }
}

bool generator_stack::stop_generator(const std::string& id)
{
    try {
        auto gen = m_generators.at(id);
        if (!gen->running()) return true;

        gen->stop();
        auto result = gen->statistics();
        m_statistics[result.id()] = result;
        gen->reset();
        return true;
    } catch (const std::out_of_range&) {
        return false;
    }
}

} // namespace openperf::network::internal
