#ifndef _OP_BLOCK_GENERATOR_STACK_HPP_
#define _OP_BLOCK_GENERATOR_STACK_HPP_

#include "block/generator.hpp"
#include "tl/expected.hpp"

namespace openperf::block::generator {

using namespace swagger::v1::model;
using json = nlohmann::json;

typedef std::shared_ptr<block_generator> block_generator_ptr;
typedef std::map<std::string, block_generator_ptr> block_generator_map;

class block_generator_stack
{
private:
    block_generator_map block_generators;

public:
    block_generator_stack(){};

    std::vector<block_generator_ptr> block_generators_list();
    tl::expected<block_generator_ptr, std::string> create_block_generator(json& block_generator_model);
    block_generator_ptr get_block_generator(std::string id) const;
    void delete_block_generator(std::string id);
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_STACK_HPP_ */