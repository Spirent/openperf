#ifndef _OP_BLOCK_DEVICE_HPP_
#define _OP_BLOCK_DEVICE_HPP_

#include "core/op_core.h"
#include <dirent.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <regex>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <vector>
#include <mntent.h>
#include "json.hpp"

#include "swagger/v1/model/BlockDevice.h"

namespace openperf::block::device {

static const std::string device_dir = "/dev";

using namespace swagger::v1::model;

uint64_t get_block_device_size(const std::string id)
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

std::string get_block_device_info(const std::string) { return ""; }

int is_block_device_usable(const std::string id)
{
    /*
     * Disallow block I/O on both the boot disk, vblk0, and the primary
     * OS partition, vblk0.1.
     */
    const char* boot_regex = "vblk0([.]1)?$";
    const std::regex id_regex(boot_regex);
    return !std::regex_match(id, id_regex);
}

bool is_raw_device(const std::string id)
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

std::unique_ptr<BlockDevice> get_block_device(std::string id)
{
    if (!is_raw_device(id)) return std::unique_ptr<BlockDevice>(nullptr);
    auto blkdev = std::make_unique<BlockDevice>();
    blkdev->setId(id);
    blkdev->setPath(id);
    blkdev->setSize(get_block_device_size(id));
    blkdev->setUsable(is_block_device_usable(id));
    blkdev->setInfo(get_block_device_info(id));

    return blkdev;
}

std::vector<std::unique_ptr<BlockDevice>> block_devices_list()
{
    DIR* dir = NULL;
    struct dirent* entry = NULL;

    if ((dir = opendir(device_dir.c_str())) == NULL) {
        OP_LOG(OP_LOG_ERROR,
               "Could not open directory %s: %s\n",
               device_dir.c_str(),
               strerror(errno));
        throw std::runtime_error("Could not open directory");
    }

    std::vector<std::unique_ptr<BlockDevice>> devices;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) continue; /* skip directories */

        if (auto blkdev = get_block_device(entry->d_name); blkdev)
            devices.push_back(std::move(blkdev));
    }

    closedir(dir);
    return devices;
}

} // namespace openperf::block::device

#endif /* _OP_BLOCK_DEVICE_HPP_ */
