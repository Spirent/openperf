#ifndef _OP_BLOCK_DEVICE_MODEL_HPP_
#define _OP_BLOCK_DEVICE_MODEL_HPP_

#include <string>
#include <atomic>

namespace openperf::block::model {

class device
{
public:
    enum state {
        UNINIT = 0,
        INIT,
        READY,
    };

protected:
    std::string m_id;
    std::string m_path;
    int64_t m_size;
    std::string m_info;
    bool m_usable;
    std::atomic_int32_t m_init_percent_complete;
    std::atomic<state> m_state;

public:
    device() = default;
    device(const device&);

    std::string get_id() const;
    void set_id(std::string_view value);

    std::string get_path() const;
    void set_path(std::string_view value);

    uint64_t get_size() const;
    void set_size(uint64_t value);

    std::string get_info() const;
    void set_info(std::string_view value);

    bool is_usable() const;
    void set_usable(bool value);

    int32_t get_init_percent_complete() const;
    void set_init_percent_complete(int32_t value);

    state get_state() const;
    void set_state(state value);
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_DEVICE_MODEL_HPP_