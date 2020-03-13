#ifndef _OP_BLOCK_GENERATOR_HPP_
#define _OP_BLOCK_GENERATOR_HPP_

#include "swagger/v1/model/BlockGenerator.h"
#include "block/worker.hpp"

namespace openperf::block::generator {

using namespace swagger::v1::model;
using namespace openperf::block::worker; 
using json = nlohmann::json;
typedef std::unique_ptr<block_worker> block_worker_ptr;

class block_generator : public BlockGenerator
{
private:
    block_worker_ptr blkworker;
    void generate_worker_config(BlockGeneratorConfig& generator_config, worker_config &config);
    
public:
    ~block_generator() {}
    block_generator(json& val);
    void start();
    void stop();
    void fromJson(json& val) override;
    
    void setConfig(std::shared_ptr<BlockGeneratorConfig> value);
    void setResourceId(std::string value);
    void setRunning(bool value);
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_HPP_ */