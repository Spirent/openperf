#ifndef _OP_BLOCK_FILE_MODEL_HPP_
#define _OP_BLOCK_FILE_MODEL_HPP_

#include <atomic>
#include <string>
#include <string_view>

namespace openperf::block::model {

class file
{
public:
    enum state_t {
        NONE = 0,
        INIT,
        READY,
    };

protected:
    std::string m_id;
    std::atomic_int64_t m_size;
    std::atomic_int32_t m_init_percent;
    std::string m_path;
    std::atomic<state_t> m_state;

public:
    file() = default;
    file(const file& other)
        : m_id(other.m_id)
        , m_size(other.m_size.load())
        , m_init_percent(other.m_init_percent.load())
        , m_path(other.m_path)
        , m_state(other.m_state.load())
    {}
    virtual ~file() = default;

    virtual void id(std::string_view value) { m_id = value; }
    virtual void size(uint64_t value) { m_size = value; }
    virtual void init_percent_complete(int32_t p) { m_init_percent = p; }
    virtual void path(std::string_view value) { m_path = value; }
    virtual void state(state_t value) { m_state = value; }

    virtual std::string id() const { return m_id; }
    virtual uint64_t size() const { return m_size; }
    virtual int32_t init_percent_complete() const { return m_init_percent; }
    virtual std::string path() const { return m_path; }
    virtual state_t state() const { return m_state; }
};
} // namespace openperf::block::model

#endif // _OP_BLOCK_FILE_MODEL_HPP_