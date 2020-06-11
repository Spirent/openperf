#include "block/generator_stack.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

namespace openperf::block::generator {

std::vector<block_generator_ptr> generator_stack::block_generators_list() const
{
    std::vector<block_generator_ptr> blkgenerators_list;
    for (const auto& blkgenerator_pair : m_block_generators) {
        blkgenerators_list.push_back(blkgenerator_pair.second);
    }

    return blkgenerators_list;
}

tl::expected<block_generator_ptr, std::string>
generator_stack::create_block_generator(
    const model::block_generator& block_generator_model,
    const std::vector<virtual_device_stack*>& vdev_stack_list)
{
    if (get_block_generator(block_generator_model.get_id()))
        return tl::make_unexpected(
            "Generator "
            + static_cast<std::string>(block_generator_model.get_id())
            + " already exists.");
    try {
        auto blkgenerator_ptr = std::make_shared<block_generator>(
            block_generator_model, vdev_stack_list);
        m_block_generators.emplace(blkgenerator_ptr->get_id(),
                                   blkgenerator_ptr);
        if (blkgenerator_ptr->is_running()) {
            m_block_results[blkgenerator_ptr->get_statistics()->get_id()] =
                blkgenerator_ptr;
        }
        return blkgenerator_ptr;
    } catch (const std::runtime_error& e) {
        return tl::make_unexpected(
            "Cannot use resource: "
            + static_cast<std::string>(
                  block_generator_model.get_resource_id()));
    }
}

block_generator_ptr
generator_stack::get_block_generator(const std::string& id) const
{
    if (m_block_generators.count(id)) return m_block_generators.at(id);
    return nullptr;
}

bool generator_stack::delete_block_generator(const std::string& id)
{
    auto gen = get_block_generator(id);
    if (!gen) return false;
    if (gen->is_running()) { stop_generator(id); }

    for (auto it = m_block_results.cbegin(); it != m_block_results.cend();) {
        auto res = (*it).second;
        ++it;
        std::visit(utils::overloaded_visitor(
                       [](const block_generator_ptr& generator) {},
                       [&](const block_generator_result_ptr& result) {
                           if (result->get_generator_id() == id)
                               delete_statistics(result->get_id());
                       }),
                   res);
    }

    return (m_block_generators.erase(id) > 0);
}

tl::expected<block_generator_result_ptr, std::string>
generator_stack::start_generator(const std::string& id)
{
    auto gen = get_block_generator(id);
    if (!gen) return nullptr;

    if (gen->is_running()) {
        return tl::make_unexpected("Generator is already in running state");
    }

    auto result = gen->start();
    m_block_results[result->get_id()] = gen;
    return result;
}

bool generator_stack::stop_generator(const std::string& id)
{
    auto gen = get_block_generator(id);
    if (!gen) return false;

    if (!gen->is_running()) { return true; }

    gen->stop();
    auto result = gen->get_statistics();
    m_block_results[result->get_id()] = result;
    gen->clear_statistics();
    return true;
}

std::vector<block_generator_result_ptr> generator_stack::list_statistics() const
{
    std::vector<block_generator_result_ptr> result_list;
    for (const auto& pair : m_block_results) {
        std::visit(utils::overloaded_visitor(
                       [&](const block_generator_ptr& generator) {
                           result_list.push_back(generator->get_statistics());
                       },
                       [&](const block_generator_result_ptr& result) {
                           result_list.push_back(result);
                       }),
                   pair.second);
    }

    return result_list;
}

block_generator_result_ptr
generator_stack::get_statistics(const std::string& id) const
{
    if (!m_block_results.count(id)) return nullptr;

    auto result = m_block_results.at(id);
    auto stat = std::visit(
        utils::overloaded_visitor(
            [&](const block_generator_ptr& generator) {
                return generator->get_statistics();
            },
            [&](const block_generator_result_ptr& res) { return res; }),
        result);

    return stat;
}

bool generator_stack::delete_statistics(const std::string& id)
{
    if (!m_block_results.count(id)) return false;

    auto result = m_block_results.at(id);
    std::visit(utils::overloaded_visitor(
                   [&](const block_generator_ptr&) {
                       // Do nothing
                   },
                   [&](const block_generator_result_ptr&) {
                       m_block_results.erase(id);
                   }),
               result);
    return true;
}

} // namespace openperf::block::generator
