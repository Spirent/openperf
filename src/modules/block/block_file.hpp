#ifndef _OP_BLOCK_FILE_HPP_
#define _OP_BLOCK_FILE_HPP_

#include "swagger/v1/model/BlockFile.h"
#include "tl/expected.hpp"

using namespace swagger::v1::model;

namespace openperf::block::file {

typedef std::map<std::string, std::shared_ptr<BlockFile>> BlockFileMap;

class block_file_stack {
private: 
    BlockFileMap block_files;

public:
    block_file_stack(){};

    std::vector<std::shared_ptr<BlockFile>> block_files_list();
    tl::expected<std::shared_ptr<BlockFile>, std::string> create_block_file(BlockFile& block_file);
    std::shared_ptr<BlockFile> get_block_file(std::string id);
    void delete_block_file(std::string id);
};

}

#endif /* _OP_BLOCK_FILE_HPP_ */