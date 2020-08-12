#ifndef _OP_BLOCK_FILE_MODEL_HPP_
#define _OP_BLOCK_FILE_MODEL_HPP_

#include <string>
#include <atomic>

namespace openperf::block::model {

class file
{
public:
    enum state {
        NONE = 0,
        INIT,
        READY,
    };

protected:
    std::string m_id;
    std::atomic_int64_t m_size;
    std::atomic_int32_t m_init_percent_complete;
    std::string m_path;
    std::atomic<state> m_state;

public:
    file() = default;
    file(const file&);

    std::string get_id() const;
    void set_id(std::string_view value);

    uint64_t get_size() const;
    void set_size(const uint64_t value);

    int32_t get_init_percent_complete() const;
    void set_init_percent_complete(const int32_t value);

    std::string get_path() const;
    void set_path(std::string_view value);

    state get_state() const;
    void set_state(const state& value);
};
} // namespace openperf::block::model

#endif // _OP_BLOCK_FILE_MODEL_HPP_