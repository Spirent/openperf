#include "block/file_stack.hpp"

namespace openperf::block::file {

std::vector<block_file_ptr> file_stack::files_list()
{
    std::vector<block_file_ptr> blkfiles_list;
    for (auto blkfile_pair : block_files) {
        blkfiles_list.push_back(blkfile_pair.second);
    }

    return blkfiles_list;
}

tl::expected<block_file_ptr, std::string>
file_stack::create_block_file(model::file& block_file_model)
{
    if (get_block_file(block_file_model.get_id()))
        return tl::make_unexpected("File " + block_file_model.get_id()
                                   + " already exists.");

    auto blkblock_file_ptr = block_file_ptr(new model::file(block_file_model));
    block_files.emplace(block_file_model.get_id(), blkblock_file_ptr);

    return blkblock_file_ptr;
}

block_file_ptr file_stack::get_block_file(std::string id)
{
    if (block_files.count(id)) return block_files.at(id);
    return nullptr;
}

void file_stack::delete_block_file(std::string id)
{
    block_files.erase(id);
}

} // namespace openperf::block::file
