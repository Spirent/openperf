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
public:
    ~device(){};
    tl::expected<int, int> vopen();
    void vclose();
    uint64_t get_size() const;
};

using device_ptr = std::shared_ptr<device>;
using device_map = std::unordered_map<std::string, device_ptr>;

static constexpr std::string_view device_dir = "/dev";

class device_stack : public virtual_device_stack
{
private:
    device_map block_devices;

    void init_device_stack();
    uint64_t get_block_device_size(std::string_view id);
    std::string get_block_device_info(std::string_view id);
    int is_block_device_usable(std::string_view id);
    bool is_raw_device(std::string_view id);

public:
    device_stack();
    device_ptr get_block_device(std::string_view id) const;
    std::shared_ptr<virtual_device>
    get_vdev(std::string_view id) const override;
    std::vector<device_ptr> block_devices_list();
};

} // namespace openperf::block::device

#endif /* _OP_BLOCK_DEVICE_STACK_HPP_ */
