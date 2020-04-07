#ifndef _OP_BLOCK_FILE_STACK_HPP_
#define _OP_BLOCK_FILE_STACK_HPP_

#include <unordered_map>
#include <memory>
#include "models/file.hpp"
#include "tl/expected.hpp"
#include "virtual_device.hpp"
#include "utils/singleton.hpp"

namespace openperf::block::file {

class file : public model::file, public virtual_device {
protected:
    void scrub_done() override;
    void scrub_update(double) override;
public:
    file(const model::file& f);
    ~file(){};
    int vopen() override;
    void vclose() override;
    uint64_t get_size() const override;
    uint64_t get_header_size() const override;
};

typedef std::shared_ptr<file> block_file_ptr;
typedef std::unordered_map<std::string, block_file_ptr> block_file_map;

class file_stack : public utils::singleton<file_stack>
{
private:
    block_file_map block_files;

public:
    file_stack() = default;

    std::vector<block_file_ptr> files_list();
    tl::expected<block_file_ptr, std::string>
    create_block_file(const model::file& block_file);
    block_file_ptr get_block_file(const std::string& id);
    void delete_block_file(const std::string& id);
};

} // namespace openperf::block::file

#endif /* _OP_BLOCK_FILE_STACK_HPP_ */