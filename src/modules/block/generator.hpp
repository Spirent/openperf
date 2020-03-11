#ifndef _OP_BLOCK_GENERATOR_HPP_
#define _OP_BLOCK_GENERATOR_HPP_

#include "swagger/v1/model/BlockGenerator.h"
#include "modules/block/worker.hpp"

namespace openperf::block::generator {

using namespace swagger::v1::model;
using namespace openperf::block::worker; 
using json = nlohmann::json;
typedef std::unique_ptr<block_worker> block_worker_ptr;

class block_generator : public BlockGenerator
{
private:
    block_worker_ptr blkworker;

public:
    ~block_generator() {
        printf("ewq\n");
    }
    block_generator(json& val);
    void fromJson(json& val) override;
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_HPP_ */