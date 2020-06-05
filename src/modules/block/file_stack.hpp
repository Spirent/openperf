#ifndef _OP_BLOCK_FILE_STACK_HPP_
#define _OP_BLOCK_FILE_STACK_HPP_

#include <unordered_map>
#include <memory>
#include "models/file.hpp"
#include "tl/expected.hpp"
#include "virtual_device.hpp"
#include "utils/singleton.hpp"

namespace openperf::block::file {

#define FILE_HEADER_TAG "This is a big, fat OP VDEV header tag!"
#define FILE_HEADER_TAG_LENGTH 40
#define FILE_HEADER_PAD_LENGTH                                       \
    512 - FILE_HEADER_TAG_LENGTH - sizeof(timesync::bintime)         \
        - sizeof(size_t)

struct file_header
{
    char tag[FILE_HEADER_TAG_LENGTH];
    timesync::bintime init_time;
    size_t size;
    uint8_t pad[FILE_HEADER_PAD_LENGTH];
} __attribute__((packed));

class file
    : public model::file
    , public virtual_device
{
private:
    std::thread m_scrub_thread;
    std::atomic_bool m_deleted;

    void scrub_done();
    void scrub_update(double);
    void scrub_worker(size_t header_size, size_t file_size);

public:
    file(const model::file& f);
    ~file();

    tl::expected<virtual_device_descriptors, int> vopen() override;
    void vclose() override;
    uint64_t get_size() const override;
    uint64_t get_header_size() const override;

    void queue_scrub();
    void terminate_scrub();
};

using block_file_ptr = std::shared_ptr<file>;
using block_file_map = std::unordered_map<std::string, block_file_ptr>;

class file_stack : public virtual_device_stack
{
private:
    block_file_map m_block_files;

public:
    file_stack() = default;
    ~file_stack() override = default;
    std::vector<block_file_ptr> files_list();
    tl::expected<block_file_ptr, std::string>
    create_block_file(const model::file& block_file);
    std::shared_ptr<virtual_device>
    get_vdev(const std::string& id) const override;
    block_file_ptr get_block_file(const std::string& id) const;
    bool delete_block_file(const std::string& id);
};

} // namespace openperf::block::file

#endif /* _OP_BLOCK_FILE_STACK_HPP_ */