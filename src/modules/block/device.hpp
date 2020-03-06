#ifndef _OP_BLOCK_DEVICE_HPP_
#define _OP_BLOCK_DEVICE_HPP_

#include "swagger/v1/model/BlockDevice.h"
#include <vector>

namespace openperf::block::device {

using namespace swagger::v1::model;

typedef std::shared_ptr<BlockDevice> BlockDevicePtr;
typedef std::map<std::string, BlockDevicePtr> BlockDeviceMap;

static const std::string device_dir = "/dev";

class block_device_stack {
private:
    BlockDeviceMap block_devices;

    void init_block_device_stack();
    uint64_t get_block_device_size(const std::string id);
    std::string get_block_device_info(const std::string);
    int is_block_device_usable(const std::string id);
    bool is_raw_device(const std::string id);

public:
    block_device_stack();
    BlockDevicePtr get_block_device(std::string id);
    std::vector<BlockDevicePtr> block_devices_list();
};

} // namespace openperf::block::device

#endif /* _OP_BLOCK_DEVICE_HPP_ */
