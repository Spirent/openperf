#ifndef _OP_BLOCK_DEVICE_STACK_HPP_
#define _OP_BLOCK_DEVICE_STACK_HPP_

#include <vector>
#include <unordered_map>
#include "models/device.hpp"
#include "virtual_device.hpp"
#include "utils/singleton.hpp"

namespace openperf::block::device {

class device
    : public virtual_device
    , public model::device
{
private:
    void scrub_update(double) override;
    void scrub_done() override;

public:
    ~device() override;

    tl::expected<virtual_device_descriptors, int> vopen() override;
    void vclose() override;
    uint64_t size() const override { return model::device::size(); }
    std::string path() const override { return model::device::path(); }

    void size(uint64_t s) override { model::device::size(s); }
    void path(std::string_view p) override { return model::device::path(p); }

    tl::expected<void, std::string> initialize();
};

class device_stack : public virtual_device_stack
{
public:
    using device_ptr = std::shared_ptr<device>;

private:
    static constexpr std::string_view device_dir = "/dev";
    std::unordered_map<std::string, device_ptr> m_block_devices;

public:
    device_stack();

    device_ptr block_device(const std::string& id) const;
    std::shared_ptr<virtual_device> vdev(const std::string& id) const override;
    std::vector<device_ptr> block_devices_list();
    tl::expected<void, std::string> initialize_device(const std::string& id);

private:
    void init_device_stack();
    uint64_t block_device_size(std::string_view id);
    std::optional<std::string> block_device_info(std::string_view id);
    bool is_raw_device(std::string_view id);
};

} // namespace openperf::block::device

#endif /* _OP_BLOCK_DEVICE_STACK_HPP_ */
