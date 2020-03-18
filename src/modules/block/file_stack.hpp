#ifndef _OP_BLOCK_FILE_STACK_HPP_
#define _OP_BLOCK_FILE_STACK_HPP_

#include <unordered_map>
#include <memory>
#include "models/file.hpp"
#include "tl/expected.hpp"
#include "models/virtual_device.hpp"
#include "utils/singleton.hpp"

namespace openperf::block::file {

class file : public model::file, public virtual_device {
public:
    file(const model::file& f);
    ~file(){};
    int vopen();
    void vclose();
};

typedef std::shared_ptr<file> block_file_ptr;
typedef std::unordered_map<std::string, block_file_ptr> block_file_map;

class file_stack : public utils::singleton<file_stack>
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