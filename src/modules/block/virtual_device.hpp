#ifndef _OP_BLOCK_VIRTUAL_DEVICE_HPP_
#define _OP_BLOCK_VIRTUAL_DEVICE_HPP_

#include <atomic>
#include <stdexcept>
#include <thread>

#include <tl/expected.hpp>

#include "modules/timesync/bintime.hpp"

namespace openperf::block {

#define VIRTUAL_DEVICE_HEADER_TAG "This is a big, fat OP VDEV header tag!"
#define VIRTUAL_DEVICE_HEADER_TAG_LENGTH 40
#define VIRTUAL_DEVICE_HEADER_PAD_LENGTH                                       \
    512 - VIRTUAL_DEVICE_HEADER_TAG_LENGTH - sizeof(timesync::bintime)         \
        - sizeof(size_t)

struct virtual_device_header
{
    char tag[VIRTUAL_DEVICE_HEADER_TAG_LENGTH];
    timesync::bintime init_time;
    size_t size;
    uint8_t pad[VIRTUAL_DEVICE_HEADER_PAD_LENGTH];
} __attribute__((packed));

struct virtual_device_descriptors
{
    int read = -1;
    int write = -1;
};

class virtual_device
{
protected:
    int m_read_fd = -1, m_write_fd = -1;
    std::atomic_bool m_scrub_aborted;
    std::thread m_scrub_thread;

public:
    virtual_device() = default;
    virtual ~virtual_device() = default;

    virtual tl::expected<virtual_device_descriptors, int> vopen() = 0;
    virtual void vclose() = 0;
    virtual uint64_t size() const = 0;
    virtual std::string path() const = 0;

    virtual uint64_t header_size() const;
    std::optional<virtual_device_descriptors> fd() const;

    void terminate_scrub();
    tl::expected<void, std::string> queue_scrub();

protected:
    void check_header(int fd, uint64_t file_size);
    void scrub_worker(int fd, size_t header_size, size_t file_size);
    int write_header(int fd, uint64_t file_size);

    virtual void scrub_update(double p){};
    virtual void scrub_done(){};
};

class virtual_device_stack
{
public:
    virtual ~virtual_device_stack() = default;

    virtual std::shared_ptr<virtual_device> vdev(const std::string&) const = 0;
};

} // namespace openperf::block

#endif