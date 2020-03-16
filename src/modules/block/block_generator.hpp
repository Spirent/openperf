#ifndef _OP_BLOCK_GENERATOR_HPP_
#define _OP_BLOCK_GENERATOR_HPP_

#include "models/generator.hpp"
#include "block/worker.hpp"

namespace openperf::block::generator {

using namespace openperf::block::worker; 
typedef std::unique_ptr<block_worker> block_worker_ptr;

class block_generator : public model::block_generator
{
private:
    block_worker_ptr blkworker;
    worker_config generate_worker_config(const model::block_generator_config& generator_config);
    
public:
    ~block_generator() {}
    block_generator(const model::block_generator& generator_model);
    void start();
    void stop();
    
    void set_сonfig(const model::block_generator_config& value);
    void set_resource_id(const std::string& value);
    void set_running(bool value);
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_HPP_ */