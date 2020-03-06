#ifndef _OP_BLOCK_GENERATOR_HPP_
#define _OP_BLOCK_GENERATOR_HPP_

#include "swagger/v1/model/BlockGenerator.h"
#include "tl/expected.hpp"

namespace openperf::block::generator {

using namespace swagger::v1::model;

typedef std::shared_ptr<BlockGenerator> BlockGeneratorPtr;
typedef std::map<std::string, BlockGeneratorPtr> BlockGeneratorMap;

class block_generator_stack
{
private:
    BlockGeneratorMap block_generators;

public:
    block_generator_stack(){};

    std::vector<BlockGeneratorPtr> block_generators_list();
    tl::expected<BlockGeneratorPtr, std::string>
    create_block_generator(BlockGenerator& block_generator);
    BlockGeneratorPtr get_block_generator(std::string id);
    void delete_block_generator(std::string id);
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_HPP_ */