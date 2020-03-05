#ifndef _OP_BLOCK_FILE_HPP_
#define _OP_BLOCK_FILE_HPP_

#include "swagger/v1/model/BlockFile.h"
#include "tl/expected.hpp"

namespace openperf::block::file {

using namespace swagger::v1::model;

typedef std::shared_ptr<BlockFile> BlockFilePtr;
typedef std::map<std::string, BlockFilePtr> BlockFileMap;

class block_file_stack {
private: 
    BlockFileMap block_files;

public:
    block_file_stack(){};

    std::vector<BlockFilePtr> block_files_list();
    tl::expected<BlockFilePtr, std::string> create_block_file(BlockFile& block_file);
    BlockFilePtr get_block_file(std::string id);
    void delete_block_file(std::string id);
};

}

#endif /* _OP_BLOCK_FILE_HPP_ */