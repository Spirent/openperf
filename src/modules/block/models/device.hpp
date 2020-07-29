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
    uint64_t m_size;
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
    virtual ~device() = default;

    virtual std::string id() const { return m_id; }
    virtual std::string path() const { return m_path; }
    virtual uint64_t size() const { return m_size; }
    virtual std::string info() const { return m_info; }
    virtual bool is_usable() const { return m_usable; }
    virtual int32_t init_percent_complete() const { return m_init_percent; }
    virtual state_t state() const { return m_state; }

    virtual void id(std::string_view value) { m_id = value; }
    virtual void path(std::string_view value) { m_path = value; }
    virtual void size(uint64_t value) { m_size = value; }
    virtual void info(std::string_view value) { m_info = value; }
    virtual void usable(bool value) { m_usable = value; }
    virtual void init_percent_complete(int32_t p) { m_init_percent = p; }
    virtual void state(state_t value) { m_state = value; }
};

} // namespace openperf::block::model

#endif // _OP_BLOCK_DEVICE_MODEL_HPP_