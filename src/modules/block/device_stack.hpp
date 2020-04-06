#ifndef _OP_BLOCK_DEVICE_STACK_HPP_
#define _OP_BLOCK_DEVICE_STACK_HPP_

#include <vector>
#include <unordered_map>
#include "models/device.hpp"
#include "virtual_device.hpp"
#include "utils/singleton.hpp"

namespace openperf::block::device {


class device : public model::device, public virtual_device {
public:
    ~device() {};
    int vopen();
    void vclose();
    uint64_t get_size() const;
};

typedef std::shared_ptr<device> device_ptr;
typedef std::unordered_map<std::string, device_ptr> device_map;

static const std::string device_dir = "/dev";

class device_stack : public utils::singleton<device_stack>{
private:
    device_map block_devices;

    void init_device_stack();
    uint64_t get_block_device_size(const std::string id);
    std::string get_block_device_info(const std::string);
    int is_block_device_usable(const std::string id);
    bool is_raw_device(const std::string id);

public:
    device_stack();
    device_ptr get_block_device(std::string id);
    std::vector<device_ptr> block_devices_list();
};

} // namespace openperf::block::device

#endif /* _OP_BLOCK_DEVICE_STACK_HPP_ */
