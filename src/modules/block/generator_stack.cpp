#include "block/generator_stack.hpp"

namespace openperf::block::generator {

std::vector<block_generator_ptr> generator_stack::block_generators_list() const
{
    std::vector<block_generator_ptr> blkgenerators_list;
    for (auto blkgenerator_pair : block_generators) {
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
        auto blkgenerator_ptr = std::make_shared<block_generator>(block_generator_model, vdev_stack_list);
        block_generators.emplace(blkgenerator_ptr->get_id(), blkgenerator_ptr);
        return blkgenerator_ptr;
    } catch (const std::runtime_error e) {
        return tl::make_unexpected(
            "Cannot open resource "
            + static_cast<std::string>(
                  block_generator_model.get_resource_id()));
    }
}

block_generator_ptr generator_stack::get_block_generator(std::string id) const
{
    if (block_generators.count(id)) return block_generators.at(id);
    return nullptr;
}

void generator_stack::delete_block_generator(std::string id)
{
    block_generators.erase(id);
}

} // namespace openperf::block::generator
