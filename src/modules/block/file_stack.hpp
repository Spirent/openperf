#ifndef _OP_BLOCK_FILE_STACK_HPP_
#define _OP_BLOCK_FILE_STACK_HPP_

#include <memory>
#include <unordered_map>

#include "tl/expected.hpp"

#include "models/file.hpp"
#include "virtual_device.hpp"

#include "framework/utils/singleton.hpp"

namespace openperf::block::file {

class file
    : public virtual_device
    , public model::file
{
private:
    void scrub_update(double) override;
    void scrub_done() override;

public:
    file(const model::file& f);
    ~file() override;

    tl::expected<virtual_device_descriptors, int> vopen() override;
    void vclose() override;

    uint64_t size() const override { return model::file::size(); };
    std::string path() const override { return model::file::path(); };
    void size(uint64_t size) override { model::file::size(size); };
    void path(std::string_view path) override { model::file::path(path); };
};

class file_stack : public virtual_device_stack
{
public:
    using block_file_ptr = std::shared_ptr<file>;

    enum class deletion_error_type { NOT_FOUND, BUSY };

private:
    std::unordered_map<std::string, block_file_ptr> m_block_files;

public:
    file_stack() = default;
    ~file_stack() override = default;

    std::vector<block_file_ptr> files_list();
    tl::expected<block_file_ptr, std::string>
    create_block_file(const model::file& block_file);
    tl::expected<void, deletion_error_type>
    delete_block_file(const std::string& id);

    std::shared_ptr<virtual_device> vdev(const std::string& id) const override;
    block_file_ptr block_file(const std::string& id) const;
};

} // namespace openperf::block::file

#endif /* _OP_BLOCK_FILE_STACK_HPP_ */