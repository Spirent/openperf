#include "block/block_file.hpp"

using namespace swagger::v1::model;

namespace openperf::block::file {

std::vector<std::shared_ptr<BlockFile>> block_file_stack::block_files_list()
{
    std::vector<std::shared_ptr<BlockFile>> blkfiles_list;
    for (auto blkfile_pair : block_files) {
        blkfiles_list.push_back(blkfile_pair.second);
    }
    
    return blkfiles_list;
}

tl::expected<std::shared_ptr<BlockFile>, std::string> block_file_stack::create_block_file(BlockFile& block_file_model)
{
    if (get_block_file(block_file_model.getId()))
        return tl::make_unexpected("File " + block_file_model.getId() + " already exists.");
    
    auto blkfile_ptr = std::shared_ptr<BlockFile>(new BlockFile(block_file_model));
    block_files.emplace(block_file_model.getId(), blkfile_ptr);
    
    return blkfile_ptr;
}

std::shared_ptr<BlockFile> block_file_stack::get_block_file(std::string id)
{
    if (block_files.count(id))
        return block_files.at(id);
    return NULL;
}

void block_file_stack::delete_block_file(std::string id)
{
    block_files.erase(id);
}

}
