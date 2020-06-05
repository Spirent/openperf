#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <vector>
#include "block/file_stack.hpp"
#include "core/op_log.h"
#include "timesync/clock.hpp"
#include "timesync/chrono.hpp"
#include "utils/random.hpp"

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

tl::expected<virtual_device_descriptors, int> file::vopen()
{
    if (m_write_fd < 0)
        m_write_fd = open(get_path().c_str(), O_WRONLY | O_CREAT | O_DSYNC, S_IRUSR | S_IWUSR);
    if (m_read_fd < 0)
        m_read_fd = open(get_path().c_str(), O_RDONLY, S_IRUSR | S_IWUSR);

    if (m_read_fd < 0 || m_write_fd < 0) {
        vclose();
        return tl::make_unexpected(errno);
    }

    return (virtual_device_descriptors) {m_read_fd, m_write_fd};
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

    if (m_read_fd >= 0)
        close_vdev(m_read_fd);
    if (m_write_fd >= 0)
        close_vdev(m_write_fd);

    m_read_fd = -1;
    m_write_fd = -1;
}

uint64_t file::get_size() const { return model::file::get_size(); }

uint64_t file::get_header_size() const
{
    return sizeof(file_header);
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


int write_header(int fd, uint64_t file_size)
{
    if (fd < 0) return -1;
    file_header header = {
        .init_time = timesync::to_bintime(
            timesync::chrono::realtime::now().time_since_epoch()),
        .size = file_size,
    };
    strncpy(header.tag,
            FILE_HEADER_TAG,
            FILE_HEADER_TAG_LENGTH);

    return (pwrite(fd, &header, sizeof(header), 0) == sizeof(header) ? 0 : -1);
}

constexpr size_t SCRUB_BUFFER_SIZE = 128 * 1024; /* 128KB */
void file::scrub_worker(size_t header_size, size_t file_size)
{
    int fd = open(get_path().c_str(), O_RDWR | O_CREAT | O_DSYNC, S_IRUSR | S_IWUSR);

    if (fd < 0) {
        OP_LOG(
            OP_LOG_ERROR, "Cannot open vdev: %s\n", strerror(errno));
        return;
    }

    ftruncate(fd, file_size);
    write_header(fd, file_size);

    auto current = header_size;
    auto page_size = getpagesize();
    while (!m_deleted && current < file_size) {
        auto buf_len = std::min(SCRUB_BUFFER_SIZE, file_size - current);
        auto file_offset = (current / page_size) * page_size;
        auto mmap_len = buf_len + current - file_offset;

        void* buf = mmap(nullptr, mmap_len, PROT_WRITE, MAP_SHARED, fd, file_offset);
        if (buf == MAP_FAILED) {
            OP_LOG(OP_LOG_ERROR, "Cannot write scrub to vdev: %s\n", strerror(errno));
            break;
        }
        utils::op_pseudo_random_fill((uint8_t*)buf + current - file_offset, buf_len);
        msync(buf, mmap_len, MS_SYNC);
        munmap(buf, mmap_len);

        current += buf_len;
        scrub_update((double)(current - header_size) / (file_size - header_size));
    }

    close(fd);
}

void file::queue_scrub()
{
    if (auto result = vopen(); !result) {
        throw std::runtime_error("Cannot open vdev device to generate scrub: "
                                 + std::string(strerror(result.error())));
    }

    struct file_header header = {};
    int read_or_err = pread(m_read_fd, &header, sizeof(header), 0);
    vclose();
    if (read_or_err == -1) {
        throw std::runtime_error("Cannot read vdev device header: "
                                 + std::string(strerror(errno)));
        return;
    } else if (read_or_err >= (int)sizeof(header)
               && strncmp(header.tag,
                          FILE_HEADER_TAG,
                          FILE_HEADER_TAG_LENGTH)
                      == 0) {
        if (header.size >= get_size()) {
            // We're done since this vdev is suitable for use
            scrub_done();
            return;
        }
    }

    m_scrub_thread = std::thread([this]() {
        scrub_worker(get_header_size(), get_size());
        scrub_done();
    });
}

void file::terminate_scrub()
{
    m_deleted = true;
    if (m_scrub_thread.joinable()) m_scrub_thread.join();
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

    if (block_file_model.get_size() <= sizeof(file_header))
        return tl::make_unexpected(
            "File size less than header size ("
            + std::to_string(sizeof(file_header)) + " bytes)");

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
