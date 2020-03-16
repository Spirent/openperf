#ifndef _OP_BLOCK_FILE_STACK_HPP_
#define _OP_BLOCK_FILE_STACK_HPP_

#include <map>
#include <memory>
#include "models/file.hpp"
#include "tl/expected.hpp"

namespace openperf::block::file {

typedef std::shared_ptr<model::file> block_file_ptr;
typedef std::map<std::string, block_file_ptr> block_file_map;

class file_stack
{
private:
    block_file_map block_files;

public:
    file_stack(){};

    std::vector<block_file_ptr> files_list();
    tl::expected<block_file_ptr, std::string>
    create_block_file(model::file& block_file);
    block_file_ptr get_block_file(std::string id);
    void delete_block_file(std::string id);
};

} // namespace openperf::block::file

#endif /* _OP_BLOCK_FILE_STACK_HPP_ */