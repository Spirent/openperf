#ifndef _OP_BLOCK_GENERATOR_STACK_HPP_
#define _OP_BLOCK_GENERATOR_STACK_HPP_

#include <unordered_map>
#include "block/block_generator.hpp"
#include "tl/expected.hpp"

namespace openperf::block::generator {

using block_generator_ptr = std::shared_ptr<block_generator>;
using block_generator_map =
    std::unordered_map<std::string, block_generator_ptr>;

class generator_stack
{
private:
    block_generator_map m_block_generators;

public:
    generator_stack(){};

    std::vector<block_generator_ptr> block_generators_list() const;
    tl::expected<block_generator_ptr, std::string> create_block_generator(
        const model::block_generator& block_generator_model,
        const std::vector<virtual_device_stack*> vdev_stack_list);
    block_generator_ptr get_block_generator(std::string_view id) const;
    void delete_block_generator(std::string_view id);
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_STACK_HPP_ */