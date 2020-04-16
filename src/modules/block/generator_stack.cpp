#include "block/generator_stack.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"
namespace openperf::block::generator {

std::vector<block_generator_ptr> generator_stack::block_generators_list() const
{
    std::vector<block_generator_ptr> blkgenerators_list;
    for (auto blkgenerator_pair : m_block_generators) {
        blkgenerators_list.push_back(blkgenerator_pair.second);
    }

    return blkgenerators_list;
}

tl::expected<block_generator_ptr, std::string>
generator_stack::create_block_generator(
    const model::block_generator& block_generator_model,
    const std::vector<virtual_device_stack*> vdev_stack_list)
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
        return blkgenerator_ptr;
    } catch (const std::runtime_error e) {
        return tl::make_unexpected(
            "Cannot open resource "
            + static_cast<std::string>(
                  block_generator_model.get_resource_id()));
    }
}

block_generator_ptr
generator_stack::get_block_generator(std::string_view id) const
{
    if (m_block_generators.count(std::string(id)))
        return m_block_generators.at(std::string(id));
    return nullptr;
}

void generator_stack::delete_block_generator(std::string_view id)
{
    auto gen = get_block_generator(id);
    if (!gen)
        return;
    if (gen->is_running()) {
        stop_generator(id);
    }

    m_block_generators.erase(std::string(id));
}

tl::expected<block_generator_result_ptr, std::string> generator_stack::start_generator(std::string_view id)
{
    auto gen = get_block_generator(id);
    if (!gen)
        return nullptr;

    if (gen->is_running()) {
        return tl::make_unexpected("Generator is already in running state");
    }

    auto result = gen->start();
    m_block_results[result->get_id()] = gen;
    return result;
}

void generator_stack::stop_generator(std::string_view id)
{
    auto gen = get_block_generator(id);
    if (!gen->is_running()) {
        return;
    }

    gen->stop();
    auto result = gen->get_statistics();
    m_block_results[result->get_id()] = result;
    gen->clear_statistics();
    return;
}

std::vector<block_generator_result_ptr> generator_stack::list_statistics() const
{
    std::vector<block_generator_result_ptr> result_list;
    for (auto pair : m_block_results) {
        std::visit(
            utils::overloaded_visitor(
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

block_generator_result_ptr generator_stack::get_statistics(std::string_view id) const
{
    if (!m_block_results.count(std::string(id)))
        return nullptr;

    auto result = m_block_results.at(std::string(id));
    std::visit(
        utils::overloaded_visitor(
            [&](const block_generator_ptr& generator) {
                return generator->get_statistics();
            },
            [&](const block_generator_result_ptr& res) {
                return res;
            }),
    result);

    return nullptr;
}

void generator_stack::delete_statistics(std::string_view id)
{
    if (!m_block_results.count(std::string(id)))
        return;

    auto result = m_block_results.at(std::string(id));
    std::visit(
        utils::overloaded_visitor(
            [&](const block_generator_ptr&) {
                // Do nothing
            },
            [&](const block_generator_result_ptr&) {
                m_block_results.erase(std::string(id));
            }),
    result);
}

} // namespace openperf::block::generator
