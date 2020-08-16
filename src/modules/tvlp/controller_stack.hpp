#ifndef _OP_TVLP_CONTROLLER_STACK_HPP_
#define _OP_TVLP_CONTROLLER_STACK_HPP_

#include <unordered_map>
#include <variant>
#include "controller.hpp"
#include "models/tvlp_config.hpp"
#include "models/tvlp_result.hpp"
#include "tl/expected.hpp"

namespace openperf::tvlp::internal {

using tvlp_controller_ptr = std::shared_ptr<controller_t>;
using tvlp_controller_map =
    std::unordered_map<std::string, tvlp_controller_ptr>;
using tvlp_result_ptr = std::shared_ptr<model::tvlp_result_t>;

class controller_stack
{
private:
    tvlp_controller_map m_controllers;

public:
    controller_stack() = default;

    std::vector<tvlp_controller_ptr> list() const;
    tl::expected<tvlp_controller_ptr, std::string>
    create(const model::tvlp_configuration_t&);
    tl::expected<tvlp_controller_ptr, std::string>
    get(const std::string& id) const;
    bool erase(const std::string& id);

    tl::expected<tvlp_result_ptr, std::string> start(const std::string&,
                                                     const time_point&);
    tl::expected<void, std::string> stop(const std::string&);
};

} // namespace openperf::tvlp::internal

#endif /* _OP_TVLP_CONTROLLER_STACK_HPP_ */
