#include <unistd.h>
#include <cstring>
#include <aio.h>
#include "virtual_device.hpp"
#include "timesync/clock.hpp"
#include "timesync/chrono.hpp"
#include "utils/random.hpp"
#include "core/op_log.h"

namespace openperf::block {

virtual_device::virtual_device()
    : m_read_fd(-1)
    , m_write_fd(-1)
{}

virtual_device::~virtual_device() { terminate_scrub(); }

int virtual_device::write_header()
{
    if (m_write_fd < 0) return -1;
    virtual_device_header header = {
        .init_time = timesync::to_bintime(
            timesync::chrono::realtime::now().time_since_epoch()),
        .size = get_size(),
    };
    strncpy(header.tag,
            VIRTUAL_DEVICE_HEADER_TAG,
            VIRTUAL_DEVICE_HEADER_TAG_LENGTH);

    return (pwrite(m_write_fd, &header, sizeof(header), 0) == sizeof(header) ? 0
                                                                       : -1);
}

void virtual_device::queue_scrub()
{
    if (auto result = vopen(); !result) {
        throw std::runtime_error("Cannot open vdev device to generate scrub: "
                                 + std::string(strerror(result.error())));
    }

    struct virtual_device_header header = {};
    int read_or_err = pread(m_read_fd, &header, sizeof(header), 0);
    vclose();
    if (read_or_err == -1) {
        throw std::runtime_error("Cannot read vdev device header: "
                                 + std::string(strerror(errno)));
        return;
    } else if (read_or_err >= (int)sizeof(header)
               && strncmp(header.tag,
                          VIRTUAL_DEVICE_HEADER_TAG,
                          VIRTUAL_DEVICE_HEADER_TAG_LENGTH)
                      == 0) {
        if (header.size >= get_size()) {
            // We're done since this vdev is suitable for use
            scrub_done();
            return;
        }
    }

    m_scrub_thread = std::thread([this]() {
        scrub_worker(block_generator_vdev_header_size, get_size());
    });
}

void virtual_device::terminate_scrub()
{
    m_deleted = true;
    if (m_scrub_thread.joinable()) m_scrub_thread.join();
}

void pseudo_random_fill(void* buffer, size_t length)
{
    auto seed = utils::random_uniform<uint32_t>(UINT32_MAX);
    auto* ptr = (uint32_t*)buffer;

    for (size_t i = 0; i < length / 4; i++) {
        uint32_t temp = (seed << 9) ^ (seed << 14);
        seed = temp ^ (temp >> 23) ^ (temp >> 18);
        *(ptr + i) = temp;
    }
}

constexpr size_t SCRUB_BUFFER_SIZE = 128 * 1024; /* 128KB */
void virtual_device::scrub_worker(size_t start, size_t stop)
{
    if (m_write_fd >= 0) {
        OP_LOG(OP_LOG_ERROR, "Cannot write scrub to vdev: device is busy\n");
        return;
    }

    if (auto result = vopen(); !result) {
        OP_LOG(
            OP_LOG_ERROR, "Cannot open vdev: %s\n", strerror(result.error()));
        return;
    }

    write_header();

    auto current = start;
    std::vector<uint8_t> buf(SCRUB_BUFFER_SIZE, 0);
    while (!m_deleted && current < stop) {
        pseudo_random_fill(buf.data(), SCRUB_BUFFER_SIZE);
        auto aio = ((aiocb){
            .aio_fildes = m_write_fd,
            .aio_offset = static_cast<off_t>(current),
            .aio_buf = buf.data(),
            .aio_nbytes = std::min(SCRUB_BUFFER_SIZE, stop - current),
            .aio_reqprio = 0,
            .aio_sigevent.sigev_notify = SIGEV_NONE,
        });
        if (aio_write(&aio) == 1) {
            if (errno == EAGAIN) { continue; }
            break;
        }
        const struct aiocb* aiocblist[] = {&aio};
        aio_suspend(aiocblist, 1, nullptr);
        if (aio_error(&aio) != 0) { continue; }
        int bytes = aio_return(&aio);
        current += bytes;
        scrub_update((double)(current - start) / (stop - start));
    }

    vclose();
    scrub_done();
}

} // namespace openperf::block