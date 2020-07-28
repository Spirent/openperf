#ifndef _OP_BLOCK_DEVICE_MODEL_HPP_
#define _OP_BLOCK_DEVICE_MODEL_HPP_

#include <atomic>
#include <string>
#include <string_view>

namespace openperf::block::model {

class device
{
public:
    enum state_t {
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
    std::atomic_int32_t m_init_percent;
    std::atomic<state_t> m_state;

public:
    device() = default;
    device(const device& other)
        : m_id(other.m_id)
        , m_path(other.m_path)
        , m_size(other.m_size)
        , m_info(other.m_info)
        , m_usable(other.m_usable)
        , m_init_percent(other.m_init_percent.load())
        , m_state(other.m_state.load())
    {}

    std::string get_id() const { return m_id; }
    void set_id(std::string_view value) { m_id = value; }

    std::string get_path() const { return m_path; }
    void set_path(std::string_view value) { m_path = value; }

    uint64_t get_size() const { return m_size; }
    void set_size(uint64_t value) { m_size = value; }

    std::string get_info() const { return m_info; }
    void set_info(std::string_view value) { m_info = value; }

    bool is_usable() const { return m_usable; }
    void set_usable(bool value) { m_usable = value; }

    int32_t get_init_percent_complete() const { return m_init_percent; }
    void set_init_percent_complete(int32_t value) { m_init_percent = value; }

    state_t get_state() const { return m_state; }
    void set_state(state_t value) { m_state = value; }
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_DEVICE_MODEL_HPP_