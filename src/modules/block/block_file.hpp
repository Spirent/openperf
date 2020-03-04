#ifndef _OP_BLOCK_FILE_HPP_
#define _OP_BLOCK_FILE_HPP_

#include "swagger/v1/model/BlockFile.h"

using namespace swagger::v1::model;

namespace openperf::block::file {
    
class block_file_stack {
private: 
    std::vector<std::shared_ptr<BlockFile>>  block_files;

public:
    block_file_stack(){};

    std::vector<std::shared_ptr<BlockFile>> block_files_list();
    std::shared_ptr<BlockFile> create_block_file(BlockFile& block_file);
    std::shared_ptr<BlockFile> get_block_file(std::string id);
    void delete_block_file(std::string id);
};

}

#endif /* _OP_BLOCK_FILE_HPP_ */