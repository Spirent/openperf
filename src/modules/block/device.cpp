#include "modules/block/device.hpp"
#include "core/op_core.h"
#include <dirent.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <regex>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <mntent.h>

namespace openperf::block::device {

block_device_stack::block_device_stack()
{
    init_block_device_stack();
}

void block_device_stack::init_block_device_stack()
{
    DIR* dir = NULL;
    struct dirent* entry = NULL;

    block_devices.clear();

    if ((dir = opendir(device_dir.c_str())) == NULL) {
        OP_LOG(OP_LOG_ERROR,
               "Could not open directory %s: %s\n",
               device_dir.c_str(),
               strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) continue; /* skip directories */

        if (!is_raw_device(entry->d_name)) continue;

        auto blkdev = new BlockDevice();
        blkdev->setId(core::to_string(core::uuid::random()));
        blkdev->setPath(device_dir + "/" + std::string(entry->d_name));
        blkdev->setSize(get_block_device_size(entry->d_name));
        blkdev->setUsable(is_block_device_usable(entry->d_name));
        blkdev->setInfo(get_block_device_info(entry->d_name));

        block_devices.emplace(blkdev->getId(), BlockDevicePtr(blkdev));
    }

    closedir(dir);
}

uint64_t block_device_stack::get_block_device_size(const std::string id)
{
    off_t nb_blocks = 0;
    char devname[PATH_MAX + 1];
    snprintf(devname, PATH_MAX, "%s/%s", device_dir.c_str(), id.c_str());

    int fd = open(devname, O_RDONLY);
    if (fd < 0) {
        OP_LOG(OP_LOG_ERROR,
               "Could not open device %s: %s\n",
               devname,
               strerror(errno));
        return (0);
    }

    if (ioctl(fd, BLKGETSIZE, &nb_blocks) == -1) {
        OP_LOG(OP_LOG_ERROR,
               "BLKGETSIZE ioctl call failed for %s (fd = %d): %s\n",
               devname,
               fd,
               strerror(errno));
        close(fd);
        return (0);
    }

    close(fd);

    return (nb_blocks << 9);
}

std::string block_device_stack::get_block_device_info(const std::string) { return ""; }

int block_device_stack::is_block_device_usable(const std::string id)
{
    /*
     * Disallow block I/O on both the boot disk, vblk0, and the primary
     * OS partition, vblk0.1.
     */
    const char* boot_regex = "vblk0([.]1)?$";
    const std::regex id_regex(boot_regex);
    return !std::regex_match(id, id_regex);
}

bool block_device_stack::is_raw_device(const std::string id)
{
    char devname[PATH_MAX + 1];
    struct stat st;

    snprintf(devname, PATH_MAX, "%s/%s", device_dir.c_str(), id.c_str());

    if ((stat(devname, &st)) != 0 || !S_ISBLK(st.st_mode))
        return (false); /* Not a block device */

    if (minor(st.st_rdev) != 0)
        return (false); /* Not an unpartioned block device */

    return (true);
}

BlockDevicePtr block_device_stack::get_block_device(std::string id)
{
    if (block_devices.count(id)) return block_devices.at(id);
    return NULL;
}

std::vector<BlockDevicePtr> block_device_stack::block_devices_list()
{
    std::vector<BlockDevicePtr> blkdevice_list;
    for (auto blkdevice_pair : block_devices) {
        blkdevice_list.push_back(blkdevice_pair.second);
    }

    return blkdevice_list;
}

} // namespace openperf::block::device