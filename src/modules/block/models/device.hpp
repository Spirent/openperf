#ifndef _OP_BLOCK_DEVICE_MODEL_HPP_
#define _OP_BLOCK_DEVICE_MODEL_HPP_

#include <string>

namespace openperf::block::model {

class device
{
protected:
    std::string m_id;
    std::string m_path;
    int64_t m_size;
    std::string m_info;
    bool m_usable;

public:
    device() = default;
    device(const device&) = default;

    std::string get_id() const;
    void set_id(std::string_view value);

    std::string get_path() const;
    void set_path(std::string_view value);

    uint64_t get_size() const;
    void set_size(const uint64_t value);

    std::string get_info() const;
    void set_info(std::string_view value);

    bool is_usable() const;
    void set_usable(const bool value);
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_DEVICE_MODEL_HPP_