#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "block/file_stack.hpp"

namespace openperf::block::file {

file::file(const model::file& f)
{
    set_id(f.get_id());
    set_path(f.get_path());
    set_size(f.get_size());
    set_init_percent_complete(0);
    set_state(model::file_state::INIT);

    queue_scrub();
}

tl::expected<int, int> file::vopen()
{
    if (fd >= 0) return fd;

    if ((fd = open(get_path().c_str(), O_RDWR | O_CREAT)) < 0) {
        return tl::make_unexpected(errno);
    }

    /* Disable readahead */
    posix_fadvise(fd, 0, 1, POSIX_FADV_RANDOM);
    fcntl(fd, F_SETFL, O_SYNC | O_RSYNC);

    return fd;
}

void file::vclose()
{
    close(fd);
    fd = -1;
}

uint64_t file::get_size() const { return model::file::get_size(); }

uint64_t file::get_header_size() const
{
    return block_generator_vdev_header_size;
};

void file::scrub_done()
{
    set_state(model::file_state::READY);
    set_init_percent_complete(100);
}

void file::scrub_update(double p) { set_init_percent_complete(static_cast<int32_t>(100 * p)); }

std::vector<block_file_ptr> file_stack::files_list()
{
    std::vector<block_file_ptr> blkfiles_list;
    for (auto blkfile_pair : block_files) {
        blkfiles_list.push_back(blkfile_pair.second);
    }

    return blkfiles_list;
}

tl::expected<block_file_ptr, std::string>
file_stack::create_block_file(const model::file& block_file_model)
{
    if (get_block_file(block_file_model.get_id()))
        return tl::make_unexpected("File " + block_file_model.get_id()
                                   + " already exists.");

    if (block_file_model.get_size() <= block_generator_vdev_header_size)
        return tl::make_unexpected(
            "File size less than header size ("
            + std::to_string(block_generator_vdev_header_size) + " bytes)");

    try {
        auto blkblock_file_ptr = std::make_shared<file>(block_file_model);
        block_files.emplace(block_file_model.get_id(), blkblock_file_ptr);
        return blkblock_file_ptr;
    } catch (const std::runtime_error e) {
        return tl::make_unexpected("Cannot create file: "
                                   + std::string(e.what()));
    }
}

std::shared_ptr<virtual_device>
file_stack::get_vdev(const std::string& id) const
{
    return get_block_file(id);
}

block_file_ptr file_stack::get_block_file(const std::string& id) const
{
    if (block_files.count(id)) return block_files.at(id);
    return nullptr;
}

void file_stack::delete_block_file(const std::string& id)
{
    block_files.erase(id);
}

} // namespace openperf::block::file
