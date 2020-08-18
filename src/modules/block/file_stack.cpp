#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <unistd.h>
#include <vector>

#include "file_stack.hpp"
#include "framework/core/op_log.h"

namespace openperf::block::file {

file::file(const model::file& f)
    : model::file(f)
{
    m_init_percent = 0;
    m_state = model::file::state_t::INIT;
}

file::~file() { terminate_scrub(); }

tl::expected<virtual_device_descriptors, int> file::open()
{
    if (m_write_fd < 0)
        m_write_fd = ::open(
            m_path.c_str(), O_WRONLY | O_CREAT | O_DSYNC, S_IRUSR | S_IWUSR);
    if (m_read_fd < 0)
        m_read_fd = ::open(m_path.c_str(), O_RDONLY, S_IRUSR | S_IWUSR);

    if (m_read_fd < 0 || m_write_fd < 0) {
        close();
        return tl::make_unexpected(errno);
    }

    return (virtual_device_descriptors){m_read_fd, m_write_fd};
}

void file::close()
{
    auto close_vdev = [this](int fd) {
        if (::close(fd) < 0) {
            OP_LOG(OP_LOG_ERROR,
                   "Cannot close file %s: %s",
                   m_path.c_str(),
                   strerror(errno));
        }
    };

    if (m_read_fd >= 0) close_vdev(m_read_fd);
    if (m_write_fd >= 0) close_vdev(m_write_fd);

    m_read_fd = -1;
    m_write_fd = -1;
}

void file::scrub_done()
{
    m_state = model::file::state_t::READY;
    m_init_percent = 100;
}

void file::scrub_update(double p)
{
    m_init_percent = static_cast<int32_t>(100 * p);
}

std::vector<file_stack::block_file_ptr> file_stack::files_list()
{
    std::vector<block_file_ptr> blkfiles_list;
    for (const auto& blkfile_pair : m_block_files) {
        blkfiles_list.push_back(blkfile_pair.second);
    }

    return blkfiles_list;
}

tl::expected<file_stack::block_file_ptr, std::string>
file_stack::create_block_file(const model::file& block_file_model)
{
    if (block_file(block_file_model.id()))
        return tl::make_unexpected("File " + block_file_model.id()
                                   + " already exists.");

    for (const auto& blkfile_pair : m_block_files) {
        if (std::filesystem::equivalent(block_file_model.path(),
                                        blkfile_pair.second->path()))
            return tl::make_unexpected("File with path "
                                       + block_file_model.path()
                                       + " already exists.");
    }

    if (block_file_model.size() <= sizeof(virtual_device_header)) {
        return tl::make_unexpected(
            "File size less than header size ("
            + std::to_string(sizeof(virtual_device_header)) + " bytes)");
    }

    auto blkblock_file_ptr = std::make_shared<file>(block_file_model);

    if (auto res = blkblock_file_ptr->queue_scrub(); !res) {
        return tl::make_unexpected("Cannot create file: "
                                   + std::string(res.error()));
    }

    m_block_files.emplace(block_file_model.id(), blkblock_file_ptr);
    return blkblock_file_ptr;
}

std::shared_ptr<virtual_device> file_stack::vdev(const std::string& id) const
{
    auto f = block_file(id);
    if (!f || f->state() != model::file::state_t::READY) return nullptr;
    return f;
}

file_stack::block_file_ptr file_stack::block_file(const std::string& id) const
{
    if (m_block_files.count(id)) return m_block_files.at(id);
    return nullptr;
}

tl::expected<void, file_stack::deletion_error_type>
file_stack::delete_block_file(const std::string& id)
{
    if (m_block_files.count(id) && m_block_files.at(id)->fd())
        return tl::make_unexpected(deletion_error_type::BUSY);

    auto f = m_block_files.at(id);
    if (remove(f->get_path().c_str()))
        OP_LOG(OP_LOG_ERROR,
               "Cannot delete block file %s from the disk: %s",
               f->get_path().c_str(),
               strerror(errno));

    if (m_block_files.erase(id) <= 0)
        return tl::make_unexpected(deletion_error_type::NOT_FOUND);

    return {};
}

} // namespace openperf::block::file
