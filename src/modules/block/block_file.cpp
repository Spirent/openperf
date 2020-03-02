#include "block/block_file.hpp"

using namespace swagger::v1::model;

namespace openperf::block::file {

std::vector<BlockFile*> block_file_stack::block_files_list()
{
    return block_files;
}

BlockFile* block_file_stack::create_block_file(BlockFile* block_file)
{
    block_files.push_back(block_file);
    return block_file;
}

BlockFile* block_file_stack::get_block_file(std::string id)
{
    for (BlockFile* block_file : block_files)
    {
        if (block_file->getId() == id)
            return block_file;
    }

    return NULL;
}

void block_file_stack::delete_block_file(std::string id)
{
    block_files.erase(std::remove_if(block_files.begin(), block_files.end(), [&id](const BlockFile* block_file)->bool
    {
        return block_file->getId() == id;
    }), block_files.end());
}

}
