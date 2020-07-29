#ifndef _OP_BLOCK_GENERATOR_STACK_HPP_
#define _OP_BLOCK_GENERATOR_STACK_HPP_

#include <unordered_map>
#include <variant>
#include "tl/expected.hpp"

#include "block_generator.hpp"
#include "models/generator_result.hpp"

namespace openperf::block::generator {

using block_generator_result_ptr =
    std::shared_ptr<model::block_generator_result>;
using block_generator_ptr = std::shared_ptr<block_generator>;

class generator_stack
{
private:
    using block_result_variant =
        std::variant<block_generator_ptr, block_generator_result_ptr>;

private:
    std::unordered_map<std::string, block_generator_ptr> m_block_generators;
    std::unordered_map<std::string, block_result_variant> m_block_results;

public:
    generator_stack() = default;

    tl::expected<block_generator_ptr, std::string> create_block_generator(
        const model::block_generator& block_generator_model,
        const std::vector<virtual_device_stack*>& vdev_stack_list);
    bool delete_block_generator(const std::string& id);
    bool delete_statistics(const std::string& id);

    tl::expected<block_generator_result_ptr, std::string>
    start_generator(const std::string& id);
    bool stop_generator(const std::string& id);

    std::vector<block_generator_ptr> block_generators_list() const;
    block_generator_ptr block_generator(const std::string& id) const;
    std::vector<block_generator_result_ptr> list_statistics() const;
    block_generator_result_ptr statistics(const std::string& id) const;
};

} // namespace openperf::block::generator

#endif /* _OP_BLOCK_GENERATOR_STACK_HPP_ */
