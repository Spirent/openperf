#include "block/generator_stack.hpp"

namespace openperf::block::generator {

std::vector<BlockGeneratorPtr> block_generator_stack::block_generators_list()
{
    std::vector<BlockGeneratorPtr> blkgenerators_list;
    for (auto blkgenerator_pair : block_generators) {
        blkgenerators_list.push_back(blkgenerator_pair.second);
    }
    
    return blkgenerators_list;
}

tl::expected<BlockGeneratorPtr, std::string> block_generator_stack::create_block_generator(BlockGenerator& block_generator_model)
{
    if (get_block_generator(block_generator_model.getId()))
        return tl::make_unexpected("File " + block_generator_model.getId() + " already exists.");
    
    auto blkgenerator_ptr = BlockGeneratorPtr(new BlockGenerator(block_generator_model));
    block_generators.emplace(block_generator_model.getId(), blkgenerator_ptr);
    
    return blkgenerator_ptr;
}

BlockGeneratorPtr block_generator_stack::get_block_generator(std::string id)
{
    if (block_generators.count(id))
        return block_generators.at(id);
    return NULL;
}

void block_generator_stack::delete_block_generator(std::string id)
{
    block_generators.erase(id);
}

}
