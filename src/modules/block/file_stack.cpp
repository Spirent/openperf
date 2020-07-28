#include <fcntl.h>
#include <filesystem>
#include <unistd.h>
#include <cstring>
#include <vector>
#include "block/file_stack.hpp"
#include "core/op_log.h"

namespace openperf::block::file {

file::file(const model::file& f)
{
    set_id(f.get_id());
    set_path(f.get_path());
    set_size(f.get_size());
    set_init_percent_complete(0);
    set_state(model::file::state_t::INIT);
}

file::~file() { terminate_scrub(); }

tl::expected<virtual_device_descriptors, int> file::vopen()
{
    if (m_write_fd < 0)
        m_write_fd = open(get_path().c_str(),
                          O_WRONLY | O_CREAT | O_DSYNC,
                          S_IRUSR | S_IWUSR);
    if (m_read_fd < 0)
        m_read_fd = open(get_path().c_str(), O_RDONLY, S_IRUSR | S_IWUSR);

    if (m_read_fd < 0 || m_write_fd < 0) {
        vclose();
        return tl::make_unexpected(errno);
    }

    return (virtual_device_descriptors){m_read_fd, m_write_fd};
}

void file::vclose()
{
    auto close_vdev = [this](int fd) {
        if (close(fd) < 0) {
            OP_LOG(OP_LOG_ERROR,
                   "Cannot close file %s: %s",
                   get_path().c_str(),
                   strerror(errno));
        }
    };

    if (m_read_fd >= 0) close_vdev(m_read_fd);
    if (m_write_fd >= 0) close_vdev(m_write_fd);

    m_read_fd = -1;
    m_write_fd = -1;
}

uint64_t file::get_size() const { return model::file::get_size(); }

std::string file::get_path() const { return model::file::get_path(); }

void file::scrub_done()
{
    set_state(model::file::state_t::READY);
    set_init_percent_complete(100);
}

void file::scrub_update(double p)
{
    set_init_percent_complete(static_cast<int32_t>(100 * p));
}

std::vector<block_file_ptr> file_stack::files_list()
{
    std::vector<block_file_ptr> blkfiles_list;
    for (const auto& blkfile_pair : m_block_files) {
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

    for (const auto& blkfile_pair : m_block_files) {
        if (std::filesystem::equivalent(block_file_model.get_path(),
                                        blkfile_pair.second->get_path()))
            return tl::make_unexpected("File with path "
                                       + block_file_model.get_path()
                                       + " already exists.");
    }

    if (block_file_model.get_size() <= sizeof(virtual_device_header))
        return tl::make_unexpected(
            "File size less than header size ("
            + std::to_string(sizeof(virtual_device_header)) + " bytes)");

    auto blkblock_file_ptr = std::make_shared<file>(block_file_model);

    if (auto res = blkblock_file_ptr->queue_scrub(); !res)
        return tl::make_unexpected("Cannot create file: "
                                   + std::string(res.error()));
    m_block_files.emplace(block_file_model.get_id(), blkblock_file_ptr);
    return blkblock_file_ptr;
}

std::shared_ptr<virtual_device>
file_stack::get_vdev(const std::string& id) const
{
    auto f = get_block_file(id);
    if (!f || f->get_state() != model::file::state_t::READY) return nullptr;
    return f;
}

block_file_ptr file_stack::get_block_file(const std::string& id) const
{
    if (m_block_files.count(id)) return m_block_files.at(id);
    return nullptr;
}

tl::expected<void, deletion_error_type>
file_stack::delete_block_file(const std::string& id)
{
    if (m_block_files.count(id) && m_block_files.at(id)->get_fd())
        return tl::make_unexpected(deletion_error_type::BUSY);

    if (m_block_files.erase(id) <= 0)
        return tl::make_unexpected(deletion_error_type::NOT_FOUND);

    return {};
}

} // namespace openperf::block::file
