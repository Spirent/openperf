
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/fs.h>
#include <regex>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <mntent.h>
#include "device_stack.hpp"
#include "core/op_core.h"

namespace openperf::block::device {

device::~device() { terminate_scrub(); }

tl::expected<virtual_device_descriptors, int> device::vopen()
{
    if (m_write_fd < 0)
        m_write_fd = open(
            get_path().c_str(), O_RDWR | O_CREAT | O_DSYNC, S_IRUSR | S_IWUSR);
    if (m_read_fd < 0)
        m_read_fd = open(get_path().c_str(), O_RDONLY, S_IRUSR | S_IWUSR);

    if (m_read_fd < 0 || m_write_fd < 0) {
        vclose();
        return tl::make_unexpected(errno);
    }

    return (virtual_device_descriptors){m_read_fd, m_write_fd};
}

void device::vclose()
{
    auto close_vdev = [this](int fd) {
        if (close(fd) < 0) {
            OP_LOG(OP_LOG_ERROR,
                   "Cannot close file %s: %s",
                   get_path().c_str(),
                   strerror(errno));
        }
    };

    if (m_read_fd >= 0) close_vdev(m_read_fd);
    if (m_write_fd >= 0) close_vdev(m_write_fd);

    m_read_fd = -1;
    m_write_fd = -1;
}

uint64_t device::get_size() const { return model::device::get_size(); }

std::string device::get_path() const { return model::device::get_path(); }

void device::scrub_done()
{
    set_state(model::device::state::READY);
    set_init_percent_complete(100);
}

void device::scrub_update(double p)
{
    set_init_percent_complete(static_cast<int32_t>(100 * p));
}

tl::expected<void, std::string> device::initialize()
{
    if (!is_usable())
        return tl::make_unexpected("Cannot initialize unusable device");

    if (get_size() <= sizeof(virtual_device_header))
        return tl::make_unexpected(
            "Device size less than header size ("
            + std::to_string(sizeof(virtual_device_header)) + " bytes)");

    if (get_state() != state::NONE)
        return tl::make_unexpected("Device is already initialized");

    if (auto res = queue_scrub(); !res) return res;

    if (get_state() == state::NONE) set_state(state::INIT);
    return {};
}

device_stack::device_stack() { init_device_stack(); }

void device_stack::init_device_stack()
{
    DIR* dir = nullptr;
    struct dirent* entry = nullptr;

    m_block_devices.clear();

    if ((dir = opendir(device_dir.data())) == nullptr) {
        OP_LOG(OP_LOG_ERROR,
               "Could not open directory %s: %s\n",
               device_dir.data(),
               strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) continue; /* skip directories */

        if (!is_raw_device(entry->d_name)) continue;

        auto blkdev = std::make_shared<device>();
        blkdev->set_id(core::to_string(core::uuid::random()));
        blkdev->set_path(std::string(device_dir) + "/"
                         + std::string(entry->d_name));
        if (const auto size = get_block_device_size(entry->d_name)) {
            blkdev->set_size(size);
            blkdev->set_usable(true);
        } else {
            blkdev->set_usable(false);
        }
        blkdev->set_state(device::state::NONE);
        blkdev->set_init_percent_complete(0);

        m_block_devices.emplace(blkdev->get_id(), blkdev);
    }

    closedir(dir);
}

uint64_t device_stack::get_block_device_size(std::string_view id)
{
    off_t nb_blocks = 0;
    char devname[PATH_MAX + 1];
    snprintf(devname, PATH_MAX, "%s/%s", device_dir.data(), id.data());

    int fd = open(devname, O_RDONLY);
    if (fd < 0) {
        const auto log_level = (errno == EACCES ? OP_LOG_DEBUG : OP_LOG_ERROR);
        OP_LOG(log_level,
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

std::optional<std::string> device_stack::get_block_device_info(std::string_view)
{
    return std::nullopt;
}

bool device_stack::is_raw_device(std::string_view id)
{
    char devname[PATH_MAX + 1];
    struct stat st;

    snprintf(devname, PATH_MAX, "%s/%s", device_dir.data(), id.data());

    if ((stat(devname, &st)) != 0 || !S_ISBLK(st.st_mode))
        return (false); /* Not a block device */

    if (minor(st.st_rdev) != 0)
        return (false); /* Not an unpartioned block device */

    return (true);
}

device_ptr device_stack::get_block_device(const std::string& id) const
{
    if (m_block_devices.count(id)) return m_block_devices.at(id);
    return nullptr;
}

std::shared_ptr<virtual_device>
device_stack::get_vdev(const std::string& id) const
{
    auto dev = get_block_device(id);
    if (!dev || !dev->is_usable()) return nullptr;
    return dev;
}

std::vector<device_ptr> device_stack::block_devices_list()
{
    std::vector<device_ptr> blkdevice_list;
    for (const auto& blkdevice_pair : m_block_devices) {
        blkdevice_list.push_back(blkdevice_pair.second);
    }

    return blkdevice_list;
}

tl::expected<void, std::string>
device_stack::initialize_device(const std::string& id)
{
    auto blkdev = get_block_device(id);

    if (!blkdev) return tl::make_unexpected("Unknown device: " + id);

    if (auto res = blkdev->initialize(); !res)
        return tl::make_unexpected("Cannot initialize device: "
                                   + std::string(res.error()));
    return {};
}

} // namespace openperf::block::device
