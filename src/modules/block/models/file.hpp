#ifndef _OP_BLOCK_FILE_MODEL_HPP_
#define _OP_BLOCK_FILE_MODEL_HPP_

#include <string>

namespace openperf::block::model {
class file
{
private:
    std::string id;
    int64_t file_size;
    int32_t init_percent_complete;
    std::string path;
    std::string state;

public:
    std::string get_id() const;
    void set_id(const std::string& value);
    
    int64_t get_file_size() const;
    void set_file_size(const int64_t value);
    
    int32_t get_init_percent_complete() const;
    void set_init_percent_complete(const int32_t value);
    
    std::string get_path() const;
    void set_path(const std::string& value);
    
    std::string get_state() const;
    void set_state(const std::string& value);
};
} // openperf::block::model

#endif // _OP_BLOCK_FILE_MODEL_HPP_