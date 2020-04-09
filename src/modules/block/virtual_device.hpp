#ifndef _OP_BLOCK_VIRTUAL_DEVICE_HPP_
#define _OP_BLOCK_VIRTUAL_DEVICE_HPP_

#include <stdexcept>
#include <atomic>
#include <thread>
#include <tl/expected.hpp>
#include "timesync/bintime.hpp"

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

const size_t block_generator_vdev_header_size = sizeof(virtual_device_header);

class virtual_device
{
protected:
    std::atomic_int fd = -1;
    std::thread scrub_thread;
    std::atomic_bool deleted;
    int write_header();
    void scrub_worker(size_t start, size_t stop);
    virtual void scrub_done(){};
    virtual void scrub_update(double){};

public:
    virtual_device();
    virtual ~virtual_device();
    virtual tl::expected<int, int> vopen() = 0;
    virtual void vclose() = 0;
    void queue_scrub();
    virtual uint64_t get_size() const = 0;
    virtual uint64_t get_header_size() const { return 0; };
    int get_fd() const { return fd; }
};

class virtual_device_stack
{
public:
    virtual ~virtual_device_stack() = default;
    virtual std::shared_ptr<virtual_device>
    get_vdev(std::string_view) const = 0;
};

} // namespace openperf::block

#endif