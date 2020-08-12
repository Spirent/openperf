#ifndef _OP_BLOCK_DEVICE_STACK_HPP_
#define _OP_BLOCK_DEVICE_STACK_HPP_

#include <vector>
#include <unordered_map>
#include "models/device.hpp"
#include "virtual_device.hpp"
#include "utils/singleton.hpp"

namespace openperf::block::device {

class device
    : public model::device
    , public virtual_device
{
private:
    void scrub_update(double) override;
    void scrub_done() override;

public:
    ~device() override;
    tl::expected<virtual_device_descriptors, int> vopen() override;
    void vclose() override;
    uint64_t get_size() const override;
    std::string get_path() const override;
    tl::expected<void, std::string> initialize();
};

using device_ptr = std::shared_ptr<device>;
using device_map = std::unordered_map<std::string, device_ptr>;

static constexpr std::string_view device_dir = "/dev";

class device_stack : public virtual_device_stack
{
private:
    device_map m_block_devices;

    void init_device_stack();
    uint64_t get_block_device_size(std::string_view id);
    std::optional<std::string> get_block_device_info(std::string_view id);
    bool is_raw_device(std::string_view id);

public:
    device_stack();
    device_ptr get_block_device(const std::string& id) const;
    std::shared_ptr<virtual_device>
    get_vdev(const std::string& id) const override;
    std::vector<device_ptr> block_devices_list();
    tl::expected<void, std::string> initialize_device(const std::string& id);
};

} // namespace openperf::block::device

#endif /* _OP_BLOCK_DEVICE_STACK_HPP_ */
