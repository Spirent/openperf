#ifndef _OP_BLOCK_FILE_MODEL_HPP_
#define _OP_BLOCK_FILE_MODEL_HPP_

#include <string>
#include <atomic>

namespace openperf::block::model {

enum file_state {
    NONE = 0,
    INIT,
    READY,
};

class file
{
protected:
    std::string id;
    std::atomic_int64_t size;
    std::atomic_int32_t init_percent_complete;
    std::string path;
    std::atomic<file_state> state;

public:
    file() = default;
    file(const file &) = default;

    std::string get_id() const;
    void set_id(const std::string& value);

    uint64_t get_size() const;
    void set_size(const uint64_t value);

    int32_t get_init_percent_complete() const;
    void set_init_percent_complete(const int32_t value);

    std::string get_path() const;
    void set_path(const std::string& value);

    file_state get_state() const;
    void set_state(const file_state& value);
};
} // openperf::block::model

#endif // _OP_BLOCK_FILE_MODEL_HPP_