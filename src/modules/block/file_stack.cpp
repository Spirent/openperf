#include <fcntl.h>
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
    set_state(model::file_state::INIT);

    queue_scrub();
}

file::~file() { terminate_scrub(); }

tl::expected<int, int> file::vopen()
{
    if (m_fd >= 0) return m_fd;

    if ((m_fd = open(get_path().c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR))
        < 0) {
        return tl::make_unexpected(errno);
    }

    /* Disable readahead */
    posix_fadvise(m_fd, 0, 1, POSIX_FADV_RANDOM);
    fcntl(m_fd, F_SETFL, O_SYNC | O_RSYNC);

    return m_fd;
}

void file::vclose()
{
    if (auto res = close(m_fd); res < 0) {
        OP_LOG(OP_LOG_ERROR,
               "Cannot close file %s: %s",
               get_path().c_str(),
               strerror(errno));
    }
    m_fd = -1;
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

    if (block_file_model.get_size() <= block_generator_vdev_header_size)
        return tl::make_unexpected(
            "File size less than header size ("
            + std::to_string(block_generator_vdev_header_size) + " bytes)");

    try {
        auto blkblock_file_ptr = std::make_shared<file>(block_file_model);
        m_block_files.emplace(block_file_model.get_id(), blkblock_file_ptr);
        return blkblock_file_ptr;
    } catch (const std::runtime_error& e) {
        return tl::make_unexpected("Cannot create file: "
                                   + std::string(e.what()));
    }
}

std::shared_ptr<virtual_device>
file_stack::get_vdev(const std::string& id) const
{
    auto f = get_block_file(id);
    if (!f || f->get_state() != model::file_state::READY) return nullptr;
    return f;
}

block_file_ptr file_stack::get_block_file(const std::string& id) const
{
    if (m_block_files.count(id)) return m_block_files.at(id);
    return nullptr;
}

bool file_stack::delete_block_file(const std::string& id)
{
    return (m_block_files.erase(id) > 0);
}

} // namespace openperf::block::file
