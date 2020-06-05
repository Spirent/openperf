#ifndef _OP_BLOCK_VIRTUAL_DEVICE_HPP_
#define _OP_BLOCK_VIRTUAL_DEVICE_HPP_

#include <stdexcept>
#include <atomic>
#include <thread>
#include <tl/expected.hpp>
#include "timesync/bintime.hpp"

namespace openperf::block {

struct virtual_device_descriptors
{
    int read = -1;
    int write = -1;
};

class virtual_device
{
protected:
    int m_read_fd = -1, m_write_fd = -1;

public:
    virtual_device(){};
    virtual ~virtual_device(){};
    virtual tl::expected<virtual_device_descriptors, int> vopen() = 0;
    virtual void vclose() = 0;
    virtual uint64_t get_size() const = 0;
    virtual uint64_t get_header_size() const { return 0; };
    std::optional<virtual_device_descriptors> get_fd() const
    {
        if (m_read_fd < 0 || m_write_fd < 0) return std::nullopt;
        return (virtual_device_descriptors){
            m_read_fd,
            m_write_fd
        };
    }
};

class virtual_device_stack
{
public:
    virtual ~virtual_device_stack() = default;
    virtual std::shared_ptr<virtual_device>
    get_vdev(const std::string&) const = 0;
};

} // namespace openperf::block

#endif