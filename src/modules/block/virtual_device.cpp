#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "virtual_device.hpp"

#include "framework/core/op_log.h"
#include "framework/utils/random.hpp"
#include "modules/timesync/clock.hpp"
#include "modules/timesync/chrono.hpp"

namespace openperf::block {

uint64_t virtual_device::header_size() const
{
    return sizeof(virtual_device_header);
}

int virtual_device::write_header(int fd, uint64_t file_size)
{
    if (fd < 0) return -1;

    virtual_device_header header = {
        .init_time = timesync::to_bintime(
            timesync::chrono::realtime::now().time_since_epoch()),
        .size = file_size,
    };

    strncpy(header.tag,
            VIRTUAL_DEVICE_HEADER_TAG,
            VIRTUAL_DEVICE_HEADER_TAG_LENGTH);

    return (pwrite(fd, &header, sizeof(header), 0) == sizeof(header) ? 0 : -1);
}

constexpr size_t SCRUB_BUFFER_SIZE = 128 * 1024; /* 128KB */
void virtual_device::scrub_worker(int fd, size_t header_size, size_t file_size)
{
    ftruncate(fd, file_size);
    write_header(fd, file_size);

    auto current = header_size;
    auto page_size = getpagesize();
    while (!m_scrub_aborted && current < file_size) {
        auto buf_len = std::min(SCRUB_BUFFER_SIZE, file_size - current);
        auto file_offset = (current / page_size) * page_size;
        auto mmap_len = buf_len + current - file_offset;

        void* buf =
            mmap(nullptr, mmap_len, PROT_WRITE, MAP_SHARED, fd, file_offset);
        if (buf == MAP_FAILED) {
            OP_LOG(OP_LOG_ERROR,
                   "Cannot write scrub to vdev: %s\n",
                   strerror(errno));
            break;
        }

        utils::op_prbs23_fill((uint8_t*)buf + current - file_offset, buf_len);
        msync(buf, mmap_len, MS_SYNC);
        munmap(buf, mmap_len);

        current += buf_len;
        scrub_update(static_cast<double>(current - header_size)
                     / static_cast<double>(file_size - header_size));
    }
}

tl::expected<void, std::string> virtual_device::queue_scrub()
{
    int fd =
        ::open(path().c_str(), O_RDWR | O_CREAT | O_DSYNC, S_IRUSR | S_IWUSR);

    if (fd < 0) { return tl::make_unexpected("Wrong file descriptor"); }

    struct virtual_device_header header = {};
    int read_or_err = pread(fd, &header, sizeof(header), 0);

    if (read_or_err == -1) {
        ::close(fd);
        return tl::make_unexpected("Cannot read header: "
                                   + std::string(strerror(errno)));
    } else if (read_or_err >= (int)sizeof(header)
               && strncmp(header.tag,
                          VIRTUAL_DEVICE_HEADER_TAG,
                          VIRTUAL_DEVICE_HEADER_TAG_LENGTH)
                      == 0) {
        if (header.size >= size()) {
            // We're done since this vdev is suitable for use
            ::close(fd);
            scrub_done();
            return {};
        }
    }

    m_scrub_thread = std::thread([this, fd]() {
        scrub_worker(fd, header_size(), size());
        ::close(fd);
        scrub_done();
    });
    return {};
}

void virtual_device::terminate_scrub()
{
    m_scrub_aborted = true;
    if (m_scrub_thread.joinable()) m_scrub_thread.join();
}

std::optional<virtual_device_descriptors> virtual_device::fd() const
{
    if (m_read_fd < 0 || m_write_fd < 0) return std::nullopt;
    return (virtual_device_descriptors){m_read_fd, m_write_fd};
}

} // namespace openperf::block
